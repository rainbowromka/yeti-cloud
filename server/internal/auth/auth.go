package auth

import (
	"crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"sync"
	"time"

	"github.com/rainbowromka/yeti-cloud/server/internal/config"
)

type TokenType string

const (
	TokenAdmin  TokenType = "admin"
	TokenInvite TokenType = "invite"
	TokenDevice TokenType = "device"
)

type TokenInfo struct {
	Hash       string    `json:"hash"`
	Type       TokenType `json:"type"`
	DeviceID   string    `json:"device_id,omitempty"`
	DeviceName string    `json:"device_name,omitempty"`
	CreatedAt  time.Time `json:"created_at"`
	ExpiresAt  time.Time `json:"expires_at,omitempty"`
	Used       bool      `json:"used"`
}

type DeviceInfo struct {
	DeviceID   string
	DeviceName string
}

type TokenStore struct {
	mu     sync.RWMutex
	tokens map[string]*TokenInfo // in-memory для invite/admin токенов (временных)
	cfg    *config.ServerConfig  // персистентное хранилище для device-токенов
}

func NewTokenStore(cfg *config.ServerConfig) *TokenStore {
	ts := &TokenStore{
		tokens: make(map[string]*TokenInfo),
		cfg:    cfg,
	}
	// Загружаем device-токены из конфига в память для быстрого доступа
	ts.loadDevicesFromConfig()
	return ts
}

// loadDevicesFromConfig загружает device-токены из конфига в память
func (ts *TokenStore) loadDevicesFromConfig() {
	ts.mu.Lock()
	defer ts.mu.Unlock()

	for _, device := range ts.cfg.Devices {
		// Восстанавливаем TokenInfo из конфига
		// Note: device_key_hash уже сохранён, но сам токен мы не храним
		// Для валидации нам нужно сравнить хеш входящего токена с device_key_hash
		// Поэтому мы сохраняем в памяти маппинг hash -> DeviceInfo
		ts.tokens[device.DeviceKeyHash] = &TokenInfo{
			Hash:       device.DeviceKeyHash,
			Type:       TokenDevice,
			DeviceID:   device.DeviceID,
			DeviceName: device.Name,
			CreatedAt:  parseTime(device.RegisteredAt),
			ExpiresAt:  time.Now().Add(365 * 24 * time.Hour), // 1 year
		}
	}
}

func parseTime(timeStr string) time.Time {
	t, _ := time.Parse(time.RFC3339, timeStr)
	return t
}

func (ts *TokenStore) GenerateAdminToken() string {
	token := generateToken()
	hash := hashToken(token)

	ts.mu.Lock()
	ts.tokens[hash] = &TokenInfo{
		Hash:      hash,
		Type:      TokenAdmin,
		CreatedAt: time.Now(),
	}
	ts.mu.Unlock()

	return token
}

func (ts *TokenStore) GenerateInviteToken() (string, time.Time) {
	token := generateToken()
	hash := hashToken(token)
	expiresAt := time.Now().Add(24 * time.Hour)

	ts.mu.Lock()
	info := &TokenInfo{
		Hash:      hash,
		Type:      TokenInvite,
		CreatedAt: time.Now(),
		ExpiresAt: expiresAt,
		Used:      false,
	}
	ts.tokens[hash] = info
	ts.mu.Unlock()

	return token, expiresAt
}

func (ts *TokenStore) GenerateDeviceToken(deviceID, deviceName string) (string, error) {
	token := generateToken()
	hash := hashToken(token)

	// Сохраняем в конфиг (персистентно)
	if err := ts.cfg.AddDevice(deviceID, hash, deviceName); err != nil {
		return "", fmt.Errorf("failed to save device: %w", err)
	}

	// Сохраняем в память
	ts.mu.Lock()
	ts.tokens[hash] = &TokenInfo{
		Hash:       hash,
		Type:       TokenDevice,
		DeviceID:   deviceID,
		DeviceName: deviceName,
		CreatedAt:  time.Now(),
		ExpiresAt:  time.Now().Add(365 * 24 * time.Hour),
	}
	ts.mu.Unlock()

	return token, nil
}

func (ts *TokenStore) ValidateAdminToken(token string) bool {
	return ts.validate(token, TokenAdmin)
}

func (ts *TokenStore) ValidateInviteToken(token string) bool {
	ts.mu.Lock()
	defer ts.mu.Unlock()

	hash := hashToken(token)
	info, ok := ts.tokens[hash]
	if !ok {
		return false
	}
	if info.Type != TokenInvite {
		return false
	}
	if info.Used {
		return false
	}
	if time.Now().After(info.ExpiresAt) {
		delete(ts.tokens, hash)
		return false
	}

	info.Used = true
	return true
}

func (ts *TokenStore) ValidateDeviceToken(token string) (*DeviceInfo, bool) {
	ts.mu.RLock()
	defer ts.mu.RUnlock()

	hash := hashToken(token)
	info, ok := ts.tokens[hash]
	if !ok {
		return nil, false
	}
	if info.Type != TokenDevice {
		return nil, false
	}
	if time.Now().After(info.ExpiresAt) {
		return nil, false
	}

	return &DeviceInfo{
		DeviceID:   info.DeviceID,
		DeviceName: info.DeviceName,
	}, true
}

func (ts *TokenStore) GetDevices() []DeviceInfo {
	ts.mu.RLock()
	defer ts.mu.RUnlock()

	var devices []DeviceInfo
	for _, info := range ts.tokens {
		if info.Type == TokenDevice {
			devices = append(devices, DeviceInfo{
				DeviceID:   info.DeviceID,
				DeviceName: info.DeviceName,
			})
		}
	}
	return devices
}

// RevokeDevice удаляет устройство по ID
func (ts *TokenStore) RevokeDevice(deviceID string) error {
	if err := ts.cfg.RemoveDevice(deviceID); err != nil {
		return err
	}

	// Удаляем из памяти
	ts.mu.Lock()
	defer ts.mu.Unlock()
	for hash, info := range ts.tokens {
		if info.Type == TokenDevice && info.DeviceID == deviceID {
			delete(ts.tokens, hash)
		}
	}
	return nil
}

func (ts *TokenStore) validate(token string, tokenType TokenType) bool {
	ts.mu.RLock()
	defer ts.mu.RUnlock()

	hash := hashToken(token)
	info, ok := ts.tokens[hash]
	if !ok {
		return false
	}
	if info.Type != tokenType {
		return false
	}
	return true
}

func generateToken() string {
	bytes := make([]byte, 32)
	rand.Read(bytes)
	return hex.EncodeToString(bytes)
}

func hashToken(token string) string {
	hash := sha256.Sum256([]byte(token))
	return "sha256:" + hex.EncodeToString(hash[:])
}
