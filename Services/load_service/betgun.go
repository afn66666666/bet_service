package main

import (
	"context"
	"log"
	"math/rand"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	"google.golang.org/grpc/metadata"

	pb "bet_service/proto"
)

type BetGun struct {
	conn   *grpc.ClientConn
	client pb.BettingServiceClient

	success    atomic.Int64
	rejected   atomic.Int64
	failed     atomic.Int64
	completed  atomic.Int64
	totalLatNs atomic.Int64
}

func NewBetGun(addr string) (*BetGun, error) {
	conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, err
	}

	return &BetGun{
		conn:   conn,
		client: pb.NewBettingServiceClient(conn),
	}, nil
}

func (g *BetGun) PlaceBet(parent context.Context, session Session, rng *rand.Rand) error {
	requestCtx, cancel := context.WithTimeout(parent, 2*time.Second)
	defer cancel()

	requestCtx = metadata.AppendToOutgoingContext(
		requestCtx,
		"authorization",
		"Bearer "+session.AccessToken,
	)

	req := &pb.PlaceBetRequest{
		UserId:  session.UserID,
		EventId: "event_1",
		Amount:  1.0,
		Outcome: []string{"home", "draw", "away"}[rng.Intn(3)],
	}

	started := time.Now()
	response, err := g.client.PlaceBet(requestCtx, req)
	g.totalLatNs.Add(time.Since(started).Nanoseconds())
	g.completed.Add(1)

	switch {
	case err != nil:
		g.failed.Add(1)
		return err
	case response.GetSuccess():
		g.success.Add(1)
	default:
		g.rejected.Add(1)
	}

	return nil
}

func (g *BetGun) report(ctx context.Context) {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			completed := g.completed.Swap(0)
			totalLatNs := g.totalLatNs.Swap(0)

			var avgLatencyMs float64
			if completed > 0 {
				avgLatencyMs = float64(totalLatNs) / float64(completed) / 1e6
			}

			log.Printf(
				"Bet RPS ~ %d placed, %d rejected, %d failed, %.2f ms avg latency",
				g.success.Swap(0),
				g.rejected.Swap(0),
				g.failed.Swap(0),
				avgLatencyMs,
			)
		}
	}
}

func (g *BetGun) Close() error {
	return g.conn.Close()
}
