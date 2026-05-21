package api

import (
	"crypto/rand"
	"encoding/hex"
)

// generateDeviceID creates a short random device identifier.
func generateDeviceID() string {
	bytes := make([]byte, 6)
	rand.Read(bytes)
	return "dev-" + hex.EncodeToString(bytes)
}
