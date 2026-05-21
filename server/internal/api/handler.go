package api

import (
	"encoding/json"
	"log"
	"net/http"

	"github.com/rainbowromka/yeti-cloud/server/internal/auth"
	"github.com/rainbowromka/yeti-cloud/server/internal/hub"
)

// Handler holds dependencies for HTTP API handlers.
type Handler struct {
	tokens   *auth.TokenStore
	hub      *hub.Hub
	adminKey string
}

// NewHandler creates a new Handler.
func NewHandler(tokens *auth.TokenStore, h *hub.Hub, adminKey string) *Handler {
	return &Handler{
		tokens:   tokens,
		hub:      h,
		adminKey: adminKey,
	}
}

// --- Request / Response types ---

type CreateInviteResponse struct {
	InviteToken string `json:"invite_token"`
	ExpiresAt   string `json:"expires_at"`
}

type RegisterDeviceRequest struct {
	InviteToken string `json:"invite_token"`
	DeviceName  string `json:"device_name"`
}

type RegisterDeviceResponse struct {
	DeviceKey string `json:"device_key"`
	DeviceID  string `json:"device_id"`
}

type DeviceEntry struct {
	DeviceID   string `json:"device_id"`
	DeviceName string `json:"device_name"`
	Online     bool   `json:"online"`
}

type ListDevicesResponse struct {
	Devices []DeviceEntry `json:"devices"`
}

// --- Handlers ---

// HandleCreateInvite generates a new invite token. Admin only.
// POST /api/invite
func (h *Handler) HandleCreateInvite(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	token, expiresAt := h.tokens.GenerateInviteToken()

	resp := CreateInviteResponse{
		InviteToken: token,
		ExpiresAt:   expiresAt.Format("2006-01-02T15:04:05Z07:00"),
	}

	writeJSON(w, http.StatusOK, resp)
	log.Printf("Invite token created, expires at %s", expiresAt)
}

// HandleRegisterDevice registers a new device using an invite token.
// POST /api/register
func (h *Handler) HandleRegisterDevice(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	var req RegisterDeviceRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.InviteToken == "" {
		http.Error(w, "invite_token is required", http.StatusBadRequest)
		return
	}
	if req.DeviceName == "" {
		req.DeviceName = "Unknown Device"
	}

	if !h.tokens.ValidateInviteToken(req.InviteToken) {
		http.Error(w, "Invalid or expired invite token", http.StatusUnauthorized)
		return
	}

	deviceID := generateDeviceID()
	deviceKey := h.tokens.GenerateDeviceToken(deviceID, req.DeviceName)

	resp := RegisterDeviceResponse{
		DeviceKey: deviceKey,
		DeviceID:  deviceID,
	}

	writeJSON(w, http.StatusOK, resp)
	log.Printf("Device registered: %s (%s)", deviceID, req.DeviceName)
}

// HandleListDevices returns all registered devices with online status. Admin only.
// GET /api/devices
func (h *Handler) HandleListDevices(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}

	registered := h.tokens.GetDevices()
	online := h.hub.GetOnlineDeviceIDs()

	devices := make([]DeviceEntry, 0, len(registered))
	for _, dev := range registered {
		entry := DeviceEntry{
			DeviceID:   dev.DeviceID,
			DeviceName: dev.DeviceName,
			Online:     contains(online, dev.DeviceID),
		}
		devices = append(devices, entry)
	}

	writeJSON(w, http.StatusOK, ListDevicesResponse{Devices: devices})
}

// --- Helpers ---

func writeJSON(w http.ResponseWriter, status int, v interface{}) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	json.NewEncoder(w).Encode(v)
}

func contains(slice []string, item string) bool {
	for _, s := range slice {
		if s == item {
			return true
		}
	}
	return false
}
