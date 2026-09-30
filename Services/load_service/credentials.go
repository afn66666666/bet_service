package main

import (
	"encoding/json"
	"fmt"
	"io"
	"os"
	"strings"
)

type Credentials struct {
	Email    string `json:"email"`
	Password string `json:"password"`
}

func loadCredentials(path string) ([]Credentials, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, fmt.Errorf("open credentials file %q: %w", path, err)
	}
	defer file.Close()

	decoder := json.NewDecoder(file)
	decoder.DisallowUnknownFields()

	var credentials []Credentials
	if err := decoder.Decode(&credentials); err != nil {
		return nil, fmt.Errorf("decode credentials file %q: %w", path, err)
	}

	var trailingData any
	if err := decoder.Decode(&trailingData); err != io.EOF {
		if err == nil {
			return nil, fmt.Errorf("credentials file %q contains data after the JSON array", path)
		}
		return nil, fmt.Errorf("decode trailing data in credentials file %q: %w", path, err)
	}

	if len(credentials) == 0 {
		return nil, fmt.Errorf("credentials file %q contains no users", path)
	}

	seenEmails := make(map[string]struct{}, len(credentials))
	for i, credentials := range credentials {
		if strings.TrimSpace(credentials.Email) == "" {
			return nil, fmt.Errorf("credentials[%d] has an empty email", i)
		}
		if credentials.Password == "" {
			return nil, fmt.Errorf("credentials[%d] has an empty password", i)
		}
		if _, exists := seenEmails[credentials.Email]; exists {
			return nil, fmt.Errorf("credentials[%d] has duplicate email %q", i, credentials.Email)
		}
		seenEmails[credentials.Email] = struct{}{}
	}

	return credentials, nil
}
