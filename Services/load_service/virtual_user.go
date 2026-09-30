package main

import (
	"context"
	"math/rand"
	"time"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

type VirtualUser struct {
	id          int
	credentials Credentials
	login       *LoginSimulator
	bets        *BetGun
	betsPerSec  float64
}

func NewVirtualUser(
	id int,
	credentials Credentials,
	login *LoginSimulator,
	bets *BetGun,
	betsPerSec float64,
) *VirtualUser {
	return &VirtualUser{
		id:          id,
		credentials: credentials,
		login:       login,
		bets:        bets,
		betsPerSec:  betsPerSec,
	}
}

func (u *VirtualUser) Run(ctx context.Context) {
	rng := rand.New(rand.NewSource(time.Now().UnixNano() + int64(u.id)))

	for ctx.Err() == nil {
		session, ok := u.loginUntilSuccess(ctx, rng)
		if !ok {
			return
		}

		for ctx.Err() == nil {
			if !waitFor(ctx, u.nextThinkTime(rng)) {
				return
			}

			err := u.bets.PlaceBet(ctx, session, rng)
			if status.Code(err) == codes.Unauthenticated {
				break
			}
		}
	}
}

func (u *VirtualUser) loginUntilSuccess(ctx context.Context, rng *rand.Rand) (Session, bool) {
	for ctx.Err() == nil {
		session, err := u.login.Login(ctx, u.credentials)
		if err == nil {
			return session, true
		}

		retryDelay := time.Duration(500+rng.Intn(1_501)) * time.Millisecond
		if !waitFor(ctx, retryDelay) {
			return Session{}, false
		}
	}

	return Session{}, false
}

func (u *VirtualUser) nextThinkTime(rng *rand.Rand) time.Duration {
	baseInterval := time.Duration(float64(time.Second) / u.betsPerSec)
	// Jitter each user's interval between 50% and 150% of the configured mean.
	return time.Duration(float64(baseInterval) * (0.5 + rng.Float64()))
}

func waitFor(ctx context.Context, duration time.Duration) bool {
	timer := time.NewTimer(duration)
	defer timer.Stop()

	select {
	case <-ctx.Done():
		return false
	case <-timer.C:
		return true
	}
}
