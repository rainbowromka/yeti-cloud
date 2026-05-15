package hub

import (
	"encoding/json"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/gorilla/websocket"
	"github.com/rainbowromka/yeti-cloud/server/internal/auth"
)

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool { return true },
}

type DeviceInfo struct {
	DeviceID   string `json:"device_id"`
	DeviceName string `json:"device_name"`
	LocalIP    string `json:"local_ip"`
	RemoteAddr string `json:"-"`
}

type Message struct {
	Type       string          `json:"type"`
	FromDevice string          `json:"from_device,omitempty"`
	Payload    json.RawMessage `json:"payload,omitempty"`
}

type Client struct {
	Device DeviceInfo
	Conn   *websocket.Conn
	Send   chan []byte
	Hub    *Hub
}

type Hub struct {
	clients    map[string]*Client
	register   chan *Client
	unregister chan *Client
	broadcast  chan *Message
	tokens     *auth.TokenStore
	mu         sync.RWMutex
	stop       chan struct{}
}

func NewHub(tokens *auth.TokenStore) *Hub {
	return &Hub{
		clients:    make(map[string]*Client),
		register:   make(chan *Client),
		unregister: make(chan *Client),
		broadcast:  make(chan *Message, 256),
		tokens:     tokens,
		stop:       make(chan struct{}),
	}
}

func (h *Hub) Run() {
	for {
		select {
		case client := <-h.register:
			h.mu.Lock()
			h.clients[client.Device.DeviceID] = client
			h.mu.Unlock()
			log.Printf("Device connected: %s (%s)", client.Device.DeviceID, client.Device.RemoteAddr)
			h.checkLocalPeers(client)

		case client := <-h.unregister:
			h.mu.Lock()
			if _, ok := h.clients[client.Device.DeviceID]; ok {
				delete(h.clients, client.Device.DeviceID)
				close(client.Send)
			}
			h.mu.Unlock()
			log.Printf("Device disconnected: %s", client.Device.DeviceID)

		case msg := <-h.broadcast:
			h.mu.RLock()
			data, _ := json.Marshal(msg)
			for id, client := range h.clients {
				if id != msg.FromDevice {
					select {
					case client.Send <- data:
					default:
					}
				}
			}
			h.mu.RUnlock()

		case <-h.stop:
			h.mu.Lock()
			for id, client := range h.clients {
				client.Conn.Close()
				delete(h.clients, id)
			}
			h.mu.Unlock()
			return
		}
	}
}

func (h *Hub) HandleWebSocket(w http.ResponseWriter, r *http.Request) {
	token := r.URL.Query().Get("token")

	dev, valid := h.tokens.ValidateDeviceToken(token)
	if !valid {
		http.Error(w, "Unauthorized", http.StatusUnauthorized)
		return
	}

	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("WebSocket upgrade failed: %v", err)
		return
	}

	client := &Client{
		Device: DeviceInfo{
			DeviceID:   dev.DeviceID,
			DeviceName: dev.DeviceName,
			LocalIP:    r.URL.Query().Get("local_ip"),
			RemoteAddr: r.RemoteAddr,
		},
		Conn: conn,
		Send: make(chan []byte, 256),
		Hub:  h,
	}

	h.register <- client

	go client.writePump()
	go client.readPump()
}

func (h *Hub) Stop() {
	close(h.stop)
}

func (h *Hub) checkLocalPeers(client *Client) {
	h.mu.RLock()
	defer h.mu.RUnlock()

	for id, other := range h.clients {
		if id == client.Device.DeviceID {
			continue
		}
		if other.Device.RemoteAddr == client.Device.RemoteAddr {
			msg := Message{
				Type:       "peer_found",
				FromDevice: other.Device.DeviceID,
			}
			payload, _ := json.Marshal(map[string]string{
				"local_ip": other.Device.LocalIP,
			})
			msg.Payload = payload

			data, _ := json.Marshal(msg)
			client.Send <- data
			return
		}
	}
}

func (c *Client) readPump() {
	defer func() {
		c.Hub.unregister <- c
		c.Conn.Close()
	}()

	c.Conn.SetReadLimit(65536)
	c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
	c.Conn.SetPongHandler(func(string) error {
		c.Conn.SetReadDeadline(time.Now().Add(60 * time.Second))
		return nil
	})

	for {
		_, raw, err := c.Conn.ReadMessage()
		if err != nil {
			break
		}

		var msg Message
		if err := json.Unmarshal(raw, &msg); err != nil {
			continue
		}

		msg.FromDevice = c.Device.DeviceID
		c.Hub.broadcast <- &msg
	}
}

func (c *Client) writePump() {
	ticker := time.NewTicker(30 * time.Second)
	defer func() {
		ticker.Stop()
		c.Conn.Close()
	}()

	for {
		select {
		case message, ok := <-c.Send:
			if !ok {
				c.Conn.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}
			c.Conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
			if err := c.Conn.WriteMessage(websocket.TextMessage, message); err != nil {
				return
			}

		case <-ticker.C:
			c.Conn.SetWriteDeadline(time.Now().Add(10 * time.Second))
			if err := c.Conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				return
			}
		}
	}
}
