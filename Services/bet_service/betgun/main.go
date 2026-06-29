package main

import (
	"context"
	"log"
	"os/signal"
	"syscall"
	"time"
)

const (
	serverAddr = "localhost:6869" // host-mapped port for bet_service (container 50052)
	numWorkers = 16
	duration   = 10 * time.Second
)

func main() {
	// Known account ids to fire bets for. TODO: load from DB / config instead
	// of hardcoding once we move past the smoke-test stage.
	userIDs := []int64{1, 2, 3, 4, 5}

	gun, err := NewBetGun(serverAddr, userIDs, numWorkers)
	if err != nil {
		log.Fatalf("failed to create bet gun: %v", err)
	}
	defer gun.Close()

	// Stop on Ctrl+C or after the fixed duration, whichever comes first.
	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()
	ctx, cancel := context.WithTimeout(ctx, duration)
	defer cancel()

	log.Printf("firing at %s with %d workers for %s", serverAddr, numWorkers, duration)
	gun.Fire(ctx)
	log.Println("done")
}
