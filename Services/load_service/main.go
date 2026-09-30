package main

import (
	"context"
	"flag"
	"log"
	"os/signal"
	"sync"
	"syscall"
	"time"
)

func main() {
	loginAddr := flag.String("login-addr", "localhost:6868", "login_service address")
	betAddr := flag.String("bet-addr", "localhost:6869", "bet_service address")
	credentialsPath := flag.String("credentials", "credentials.json", "path to test user credentials JSON")
	userCount := flag.Int("users", 0, "number of virtual users (0 means all credentials)")
	betsPerSec := flag.Float64("bets-per-second", 8, "mean number of bets per user per second")
	duration := flag.Duration("duration", 10*time.Second, "scenario duration")
	rampUp := flag.Duration("ramp-up", 5*time.Second, "time over which virtual users are started")
	flag.Parse()

	credentials, err := loadCredentials(*credentialsPath)
	if err != nil {
		log.Fatal(err)
	}

	activeUserCount := *userCount
	if activeUserCount == 0 {
		activeUserCount = len(credentials)
	}
	if activeUserCount < 1 || activeUserCount > len(credentials) {
		log.Fatalf("users must be between 1 and %d (number of loaded credentials)", len(credentials))
	}
	if *betsPerSec <= 0 {
		log.Fatal("bets-per-second must be greater than zero")
	}

	loginSimulator, err := NewLoginSimulator(*loginAddr)
	if err != nil {
		log.Fatalf("create login client: %v", err)
	}
	defer loginSimulator.Close()

	betGun, err := NewBetGun(*betAddr)
	if err != nil {
		log.Fatalf("create betting client: %v", err)
	}
	defer betGun.Close()

	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()
	ctx, cancel := context.WithTimeout(ctx, *duration)
	defer cancel()

	go loginSimulator.report(ctx)
	go betGun.report(ctx)

	log.Printf(
		"loaded %d credentials; starting %d virtual users, %.2f bets/sec per user, target up to %.0f RPS",
		len(credentials),
		activeUserCount,
		*betsPerSec,
		float64(activeUserCount)**betsPerSec,
	)

	var wg sync.WaitGroup
	startDelay := time.Duration(0)
	if activeUserCount > 1 {
		startDelay = *rampUp / time.Duration(activeUserCount-1)
	}

	for i := 0; i < activeUserCount; i++ {
		if i > 0 && !waitFor(ctx, startDelay) {
			break
		}

		user := NewVirtualUser(i, credentials[i], loginSimulator, betGun, *betsPerSec)
		wg.Add(1)
		go func() {
			defer wg.Done()
			user.Run(ctx)
		}()
	}

	wg.Wait()
	log.Println("scenario finished")
}
