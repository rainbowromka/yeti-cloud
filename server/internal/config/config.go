package config

import (
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
)

const ConfigFileName = "yeti-server-config.json"

type ServerConfig struct {
	AdminKey string `json:"admin_key"`
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
	}

	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		log.Fatalf("Failed to marshal config: %v", err)
	}

	if err := os.WriteFile(path, data, 0644); err != nil {
		log.Printf("Warning: could not write config file: %v", err)
	} else {
		log.Printf("Saved config to %s", path)
	}

	fmt.Printf("=== ADMIN KEY: %s ===\n", cfg.AdminKey)
	return cfg
}