package main

import (
	"context"
	"fmt"

	"github.com/jackc/pgx/v5/pgxpool"

	pb "bet_service/proto"
)

// BettingServer implements pb.BettingServiceServer.
type BettingServer struct {
	pb.UnimplementedBettingServiceServer

	pool *pgxpool.Pool
}

func NewBettingServer(pool *pgxpool.Pool) *BettingServer {
	return &BettingServer{pool: pool}
}

const noopMode = true

// PlaceBet validates the user's balance and records a bet.

func (s *BettingServer) PlaceBet(ctx context.Context, req *pb.PlaceBetRequest) (*pb.PlaceBetResponse, error) {
	if noopMode {
		// Mock response: skip the DB entirely, pretend the bet went through.
		return &pb.PlaceBetResponse{Success: true, NewBalance: 1000}, nil
	}

	transaction, err := s.pool.Begin(ctx)
	if err != nil {
		fmt.Println(err.Error())
		return &pb.PlaceBetResponse{Success: false, Error: err.Error()}, nil
	}
	defer transaction.Rollback(ctx)
	var balance float64
	//load balance
	err = transaction.QueryRow(ctx, "SELECT balance FROM users WHERE id = $1 FOR UPDATE", req.UserId).Scan(&balance)
	if err != nil {
		return &pb.PlaceBetResponse{Success: false, Error: err.Error()}, nil
	}

	if balance < req.Amount {
		return &pb.PlaceBetResponse{Success: false, Error: "insufficient balance"}, nil
	}

	//update balance
	newBalance := balance - req.Amount
	_, err = transaction.Exec(ctx, "UPDATE users SET balance = $1 WHERE id = $2", newBalance, req.UserId)
	if err != nil {
		return &pb.PlaceBetResponse{Success: false, Error: err.Error()}, nil
	}

	//adding bet
	_, err = transaction.Exec(ctx, "INSERT INTO bets(user_id, event_id, amount, outcome) VALUES ($1, $2, $3, $4)",
		req.UserId, req.EventId, req.Amount, req.Outcome)
	if err != nil {
		return &pb.PlaceBetResponse{Success: false, Error: err.Error()}, nil
	}

	if err := transaction.Commit(ctx); err != nil {
		return &pb.PlaceBetResponse{Success: false, Error: err.Error()}, nil
	}
	return &pb.PlaceBetResponse{Success: true, NewBalance: newBalance}, nil
}
