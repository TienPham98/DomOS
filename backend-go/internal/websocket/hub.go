package websocket

import (
	"encoding/json"
	"sync"
	"time"

	"github.com/gofiber/websocket/v2"

	"github.com/domos/backend-go/pkg/logger"
)

// Message is the envelope sent to every connected websocket client.
type Message struct {
	Event     string      `json:"event"`
	Payload   interface{} `json:"payload"`
	Timestamp time.Time   `json:"timestamp"`
}

// Hub keeps track of connected dashboard websocket clients and fans out
// broadcast events (device online/offline, OTA progress, AI status, etc.)
// to all of them.
type Hub struct {
	mu      sync.RWMutex
	clients map[*websocket.Conn]bool
}

func NewHub() *Hub {
	return &Hub{clients: make(map[*websocket.Conn]bool)}
}

func (h *Hub) register(conn *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.clients[conn] = true
}

func (h *Hub) unregister(conn *websocket.Conn) {
	h.mu.Lock()
	defer h.mu.Unlock()
	delete(h.clients, conn)
	_ = conn.Close()
}

// Broadcast sends an event to every connected client. Implements
// mqtt.Broadcaster.
func (h *Hub) Broadcast(event string, payload interface{}) {
	msg := Message{Event: event, Payload: payload, Timestamp: time.Now()}
	data, err := json.Marshal(msg)
	if err != nil {
		logger.Error.Printf("websocket: failed to marshal broadcast message: %v", err)
		return
	}

	h.mu.RLock()
	defer h.mu.RUnlock()

	for conn := range h.clients {
		if err := conn.WriteMessage(websocket.TextMessage, data); err != nil {
			logger.Warn.Printf("websocket: write failed, dropping client: %v", err)
			go h.unregister(conn)
		}
	}
}

// Handle is the per-connection loop: register on connect, read (and
// discard/ping-pong) until the client disconnects, then unregister.
func (h *Hub) Handle(conn *websocket.Conn) {
	h.register(conn)
	defer h.unregister(conn)

	for {
		if _, _, err := conn.ReadMessage(); err != nil {
			break
		}
	}
}
