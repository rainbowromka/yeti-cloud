package config

import (
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"
)

const ConfigFileName = "yeti-server-config.json"

type DeviceInfo struct {
	DeviceID      string `json:"device_id"`
	DeviceKeyHash string `json:"device_key_hash"` // SHA-256 of device_key
	Name          string `json:"name"`
	RegisteredAt  string `json:"registered_at"`
}

type ServerConfig struct {
	AdminKey string       `json:"admin_key"`
	Devices  []DeviceInfo `json:"devices"`
}

func generateAdminKey() string {
	bytes := make([]byte, 32)
	if _, err := rand.Read(bytes); err != nil {
		log.Fatalf("Failed to generate admin key: %v", err)
	}
	return hex.EncodeToString(bytes)
}

func configPath() string {
	exe, err := os.Executable()
	if err != nil {
		return ConfigFileName
	}
	return filepath.Join(filepath.Dir(exe), ConfigFileName)
}

func LoadOrCreate() *ServerConfig {
	path := configPath()

	if data, err := os.ReadFile(path); err == nil {
		var cfg ServerConfig
		if err := json.Unmarshal(data, &cfg); err == nil && cfg.AdminKey != "" {
			log.Printf("Loaded config from %s", path)
			return &cfg
		}
	}

	log.Printf("Config file not found at %s, generating new admin key...", path)
	cfg := &ServerConfig{
		AdminKey: generateAdminKey(),
		Devices:  []DeviceInfo{},
	}

	if err := cfg.Save(); err != nil {
		log.Printf("Warning: could not write config file: %v", err)
	} else {
		log.Printf("Saved config to %s", path)
	}

	fmt.Printf("=== ADMIN KEY: %s ===\n", cfg.AdminKey)
	return cfg
}

func (c *ServerConfig) Save() error {
	path := configPath()
	data, err := json.MarshalIndent(c, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal config: %w", err)
	}
	if err := os.WriteFile(path, data, 0644); err != nil {
		return fmt.Errorf("failed to write config: %w", err)
	}
	return nil
}

func (c *ServerConfig) FindDeviceByKeyHash(keyHash string) *DeviceInfo {
	for i := range c.Devices {
		if c.Devices[i].DeviceKeyHash == keyHash {
			return &c.Devices[i]
		}
	}
	return nil
}

func (c *ServerConfig) FindDeviceByID(deviceID string) *DeviceInfo {
	for i := range c.Devices {
		if c.Devices[i].DeviceID == deviceID {
			return &c.Devices[i]
		}
	}
	return nil
}

func (c *ServerConfig) AddDevice(deviceID, keyHash, name string) error {
	// Check if already exists
	if c.FindDeviceByKeyHash(keyHash) != nil {
		return fmt.Errorf("device with this key already exists")
	}
	if c.FindDeviceByID(deviceID) != nil {
		return fmt.Errorf("device with this ID already exists")
	}

	c.Devices = append(c.Devices, DeviceInfo{
		DeviceID:      deviceID,
		DeviceKeyHash: keyHash,
		Name:          name,
		RegisteredAt:  time.Now().UTC().Format(time.RFC3339),
	})
	return c.Save()
}

func (c *ServerConfig) RemoveDevice(deviceID string) error {
	for i, d := range c.Devices {
		if d.DeviceID == deviceID {
			c.Devices = append(c.Devices[:i], c.Devices[i+1:]...)
			return c.Save()
		}
	}
	return fmt.Errorf("device not found: %s", deviceID)
}
