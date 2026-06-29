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

// PlaceBet validates the user's balance and records a bet.
//
// TODO: implement inside a single transaction (pool.Begin):
//  1. SELECT balance FROM users WHERE id = $1 FOR UPDATE  — locks the row,
//     prevents two concurrent bets from both passing the balance check.
//  2. if balance < amount -> rollback, return success=false.
//  3. UPDATE users SET balance = balance - $amount WHERE id = $1.
//  4. INSERT INTO bets(user_id, event_id, amount, outcome) VALUES (...).
//  5. commit, return success=true with the new balance.
func (s *BettingServer) PlaceBet(ctx context.Context, req *pb.PlaceBetRequest) (*pb.PlaceBetResponse, error) {

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
