package api

import (
	"net/http"
	"strings"
)

// AdminAuthMiddleware checks the Authorization header for a valid admin key.
// Format: Authorization: Bearer <admin_key>
func AdminAuthMiddleware(adminKey string) func(http.Handler) http.Handler {
	return func(next http.Handler) http.Handler {
		return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			authHeader := r.Header.Get("Authorization")

			if authHeader == "" {
				http.Error(w, `{"error":"missing Authorization header"}`, http.StatusUnauthorized)
				return
			}

			parts := strings.SplitN(authHeader, " ", 2)
			if len(parts) != 2 || !strings.EqualFold(parts[0], "Bearer") {
				http.Error(w, `{"error":"invalid Authorization format, expected: Bearer <token>"}`, http.StatusUnauthorized)
				return
			}

			token := parts[1]
			if token != adminKey {
				http.Error(w, `{"error":"invalid admin key"}`, http.StatusUnauthorized)
				return
			}

			next.ServeHTTP(w, r)
		})
	}
}
