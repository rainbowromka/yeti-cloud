package main

import (
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"github.com/rainbowromka/yeti-cloud/server/internal/api"
	"github.com/rainbowromka/yeti-cloud/server/internal/auth"
	"github.com/rainbowromka/yeti-cloud/server/internal/config"
	"github.com/rainbowromka/yeti-cloud/server/internal/hub"
)

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

	// Load or create server config with admin key
	cfg := config.LoadOrCreate()

	tokenStore := auth.NewTokenStore()
	wsHub := hub.NewHub(tokenStore, cfg.AdminKey)

	go wsHub.Run()

	// HTTP API
	apiHandler := api.NewHandler(tokenStore, wsHub, cfg.AdminKey)
	adminMiddleware := api.AdminAuthMiddleware(cfg.AdminKey)

	// WebSocket
	http.HandleFunc("/ws", wsHub.HandleWebSocket)

	// Health check
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(`{"status":"ok"}`))
	})

	// Public — registration by invite token
	http.HandleFunc("/api/register", apiHandler.HandleRegisterDevice)

	// Admin-only — requires Authorization: Bearer <admin_key>
	http.Handle("/api/invite", adminMiddleware(http.HandlerFunc(apiHandler.HandleCreateInvite)))
	http.Handle("/api/devices", adminMiddleware(http.HandlerFunc(apiHandler.HandleListDevices)))

	go func() {
		log.Printf("Yeti Server starting on :%s", port)
		log.Printf("  WebSocket:  ws://localhost:%s/ws", port)
		log.Printf("  Health:     http://localhost:%s/health", port)
		log.Printf("  API:        http://localhost:%s/api/*", port)
		if err := http.ListenAndServe(":"+port, nil); err != nil {
			log.Fatalf("Server failed: %v", err)
		}
	}()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit

	log.Println("Shutting down server...")
	wsHub.Stop()
	log.Println("Server stopped")
}
