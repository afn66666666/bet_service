package main

import (
	"context"
	"flag"
	"log"
	"os/signal"
	"syscall"
	"time"
)

func main() {
	addr := flag.String("addr", "localhost:6869", "bet_service address (host-mapped port for container 50052)")
	workers := flag.Int("workers", 16, "number of concurrent worker goroutines (in-flight requests)")
	duration := flag.Duration("duration", 10*time.Second, "how long to fire before stopping")
	flag.Parse()

	// Known account ids to fire bets for. TODO: load from DB / config instead
	// of hardcoding once we move past the smoke-test stage.
	userIDs := []int64{1, 2, 3, 4, 5}

	gun, err := NewBetGun(*addr, userIDs, *workers)
	if err != nil {
		log.Fatalf("failed to create bet gun: %v", err)
	}
	defer gun.Close()

	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()
	ctx, cancel := context.WithTimeout(ctx, *duration)
	defer cancel()

	log.Printf("firing at %s with %d workers for %s", *addr, *workers, *duration)
	gun.Fire(ctx)
	log.Println("done")
}
