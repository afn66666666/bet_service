package main

import (
	"context"
	"log"
	"math/rand"
	"sync"
	"sync/atomic"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	pb "bet_service/proto"
)

type BetGun struct {
	addr       string
	userIDs    []int64
	numWorkers int

	conn   *grpc.ClientConn
	client pb.BettingServiceClient

	// Counters, sampled once per second by the reporter.
	success    atomic.Int64 // PlaceBetResponse.Success == true
	rejected   atomic.Int64 // RPC ok, but Success == false (e.g. insufficient balance)
	failed     atomic.Int64 // transport / status error
	completed  atomic.Int64 // all of the above, for latency averaging
	totalLatNs atomic.Int64
}

// NewBetGun dials the server and prepares the client. userIDs are the known
// account ids the gun will spread bets across.
func NewBetGun(addr string, userIDs []int64, numWorkers int) (*BetGun, error) {
	conn, err := grpc.NewClient(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return nil, err
	}

	return &BetGun{
		addr:       addr,
		userIDs:    userIDs,
		numWorkers: numWorkers,
		conn:       conn,
		client:     pb.NewBettingServiceClient(conn),
	}, nil
}

// Fire launches numWorkers goroutines. Each sends PlaceBet in a tight loop
// until ctx is cancelled. A separate reporter prints throughput every second.
func (g *BetGun) Fire(ctx context.Context) {
	var wg sync.WaitGroup

	for i := 0; i < g.numWorkers; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			g.worker(ctx)
		}()
	}

	go g.report(ctx)

	wg.Wait()
}

// worker fires requests back-to-back until the context is done.
func (g *BetGun) worker(ctx context.Context) {
	rng := rand.New(rand.NewSource(time.Now().UnixNano()))

	for {
		select {
		case <-ctx.Done():
			return
		default:
			g.sendOne(ctx, rng)
		}
	}
}

// sendOne builds a bet, sends it, times it, and updates the counters.
func (g *BetGun) sendOne(ctx context.Context, rng *rand.Rand) {
	req := g.generateBet(rng)

	start := time.Now()
	resp, err := g.client.PlaceBet(ctx, req)
	g.totalLatNs.Add(time.Since(start).Nanoseconds())
	g.completed.Add(1)

	switch {
	case err != nil:
		g.failed.Add(1)
	case resp.GetSuccess():
		g.success.Add(1)
	default:
		g.rejected.Add(1)
	}
}

// generateBet produces a request for a random known user.

func (g *BetGun) generateBet(rng *rand.Rand) *pb.PlaceBetRequest {
	userID := g.userIDs[rng.Intn(len(g.userIDs))]

	return &pb.PlaceBetRequest{
		UserId:  userID,
		EventId: "event_1",
		Amount:  1.0,
		Outcome: "home",
	}
}

// report prints aggregate stats once per second and resets the counters,
// mirroring StressTester::testUserService output.
func (g *BetGun) report(ctx context.Context) {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			comp := g.completed.Swap(0)
			var avgMs float64
			if comp > 0 {
				avgMs = float64(g.totalLatNs.Swap(0)) / float64(comp) / 1e6
			}
			log.Printf("RPS ~ %d placed, %d rejected, %d failed, %.2f ms avg latency",
				g.success.Swap(0), g.rejected.Swap(0), g.failed.Swap(0), avgMs)
		}
	}
}

// Close releases the gRPC connection.
func (g *BetGun) Close() error {
	return g.conn.Close()
}
