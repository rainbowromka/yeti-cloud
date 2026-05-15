package main

import (
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"

	"github.com/rainbowromka/yeti-cloud/server/internal/auth"
	"github.com/rainbowromka/yeti-cloud/server/internal/hub"
)

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}

	tokenStore := auth.NewTokenStore()
	wsHub := hub.NewHub(tokenStore)

	go wsHub.Run()

	http.HandleFunc("/ws", wsHub.HandleWebSocket)
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte(`{"status":"ok"}`))
	})

	go func() {
		log.Printf("Yeti Server starting on :%s", port)
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
