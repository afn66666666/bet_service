package main

import (
	"context"
	loginv1 "load_service/proto/login/v1"
	"log"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/status"
)

type Session struct {
	UserID      int64
	AccessToken string
}

type LoginSimulator struct {
	conn   *grpc.ClientConn
	client loginv1.LoginServiceClient

	authorized   atomic.Int64
	unauthorized atomic.Int64
	failed       atomic.Int64
	completed    atomic.Int64
	totalLatNs   atomic.Int64
}

func NewLoginSimulator(addr string) (*LoginSimulator, error) {
	conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, err
	}

	return &LoginSimulator{
		conn:   conn,
		client: loginv1.NewLoginServiceClient(conn),
	}, nil
}

func (ls *LoginSimulator) Login(parent context.Context, credentials Credentials) (Session, error) {
	requestCtx, cancel := context.WithTimeout(parent, 2*time.Second)
	defer cancel()

	req := &loginv1.LoginRequest{
		Email:    credentials.Email,
		Password: credentials.Password,
	}

	started := time.Now()
	response, err := ls.client.Login(requestCtx, req)
	ls.totalLatNs.Add(time.Since(started).Nanoseconds())
	ls.completed.Add(1)

	if err != nil {
		switch status.Code(err) {
		case codes.NotFound, codes.Unauthenticated:
			ls.unauthorized.Add(1)
		default:
			ls.failed.Add(1)
		}
		return Session{}, err
	}

	ls.authorized.Add(1)
	return Session{
		UserID:      response.GetUserId(),
		AccessToken: response.GetAccessToken(),
	}, nil
}

func (ls *LoginSimulator) report(ctx context.Context) {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			completed := ls.completed.Swap(0)
			totalLatNs := ls.totalLatNs.Swap(0)

			var avgLatencyMs float64
			if completed > 0 {
				avgLatencyMs = float64(totalLatNs) / float64(completed) / 1e6
			}

			log.Printf(
				"Login RPS ~ %d authorized, %d unauthorized, %d failed, %.2f ms avg latency",
				ls.authorized.Swap(0),
				ls.unauthorized.Swap(0),
				ls.failed.Swap(0),
				avgLatencyMs,
			)
		}
	}
}

func (ls *LoginSimulator) Close() error {
	return ls.conn.Close()
}
