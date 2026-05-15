package auth

import (
	"crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"sync"
	"time"
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
	tokens map[string]*TokenInfo // hash → info
}

func NewTokenStore() *TokenStore {
	ts := &TokenStore{
		tokens: make(map[string]*TokenInfo),
	}
	return ts
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

func (ts *TokenStore) GenerateInviteToken() string {
	token := generateToken()
	hash := hashToken(token)

	ts.mu.Lock()
	ts.tokens[hash] = &TokenInfo{
		Hash:      hash,
		Type:      TokenInvite,
		CreatedAt: time.Now(),
		ExpiresAt: time.Now().Add(24 * time.Hour),
		Used:      false,
	}
	ts.mu.Unlock()

	return token
}

func (ts *TokenStore) GenerateDeviceToken(deviceID, deviceName string) string {
	token := generateToken()
	hash := hashToken(token)

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

	return token
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
