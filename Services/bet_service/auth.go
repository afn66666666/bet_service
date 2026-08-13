package main

import (
	"context"
	"crypto/rsa"
	"fmt"
	"os"
	"strconv"
	"strings"

	"github.com/golang-jwt/jwt/v5"
	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/metadata"
	"google.golang.org/grpc/status"
)

const (
	jwtIssuer   = "login_service"
	jwtAudience = "bet_service"
)

type userIDContextKey struct{}

type TokenVerifier struct {
	publicKey *rsa.PublicKey
}

func CreateTokenVerifier(pkeyPath string) (*TokenVerifier, error) {
	pkeyRawData, err := os.ReadFile(pkeyPath)
	if err != nil {
		return nil, fmt.Errorf("JWT pkey read error  : %w", err)
	}
	pKey, err := jwt.ParseRSAPublicKeyFromPEM(pkeyRawData)

	if err != nil {
		return nil, fmt.Errorf("parse JWT public key: %w", err)
	}

	return &TokenVerifier{
		publicKey: pKey,
	}, nil
}

func (v *TokenVerifier) Verify(rawToken string) (int64, error) {
	claims := &jwt.RegisteredClaims{}

	token, err := jwt.ParseWithClaims(rawToken, claims, func(token *jwt.Token) (any, error) {
		return v.publicKey, nil
	},
		jwt.WithValidMethods([]string{jwt.SigningMethodRS256.Alg()}),
		jwt.WithIssuer(jwtIssuer),
		jwt.WithAudience(jwtAudience),
		jwt.WithExpirationRequired())
	if err != nil {
		return 0, fmt.Errorf("verify JWT: %w", err)
	}

	if !token.Valid {
		return 0, fmt.Errorf("invalid JWT")
	}

	userID, err := strconv.ParseInt(claims.Subject, 10, 64)
	if err != nil || userID <= 0 {
		return 0, fmt.Errorf("invalid JWT subject")
	}

	return userID, nil
}

func AuthInterceptor(verifier *TokenVerifier) grpc.UnaryServerInterceptor {
	return func(
		ctx context.Context,
		req any,
		info *grpc.UnaryServerInfo,
		handler grpc.UnaryHandler,
	) (any, error) {
		rawToken, err := bearerTokenFromMetadata(ctx)
		if err != nil {
			return nil, status.Error(codes.Unauthenticated, err.Error())
		}

		userID, err := verifier.Verify(rawToken)
		if err != nil {
			return nil, status.Error(codes.Unauthenticated, "invalid access token")
		}

		ctx = context.WithValue(ctx, userIDContextKey{}, userID)
		return handler(ctx, req)
	}
}

func bearerTokenFromMetadata(ctx context.Context) (string, error) {
	md, ok := metadata.FromIncomingContext(ctx)
	if !ok {
		return "", fmt.Errorf("missing authorization metadata")
	}

	values := md.Get("authorization")
	if len(values) != 1 {
		return "", fmt.Errorf("exactly one authorization value is required")
	}

	parts := strings.Fields(values[0])
	if len(parts) != 2 || !strings.EqualFold(parts[0], "Bearer") || parts[1] == "" {
		return "", fmt.Errorf("authorization must use Bearer scheme")
	}

	return parts[1], nil
}

func UserIDFromContext(ctx context.Context) (int64, bool) {
	userID, ok := ctx.Value(userIDContextKey{}).(int64)
	return userID, ok
}
