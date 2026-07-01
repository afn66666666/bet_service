package main

import (
	"context"
	"log"
	"net"
	"os/signal"
	"syscall"

	"github.com/jackc/pgx/v5/pgxpool"
	"google.golang.org/grpc"

	pb "bet_service/proto"
)

const listenAddr = "0.0.0.0:50052"

func main() {
	ctx, stop := signal.NotifyContext(context.Background(), syscall.SIGINT, syscall.SIGTERM)
	defer stop()

	// In NOOP mode PlaceBet never touches the DB, so skip the pool entirely —
	// the container then runs without a reachable Postgres.
	var pool *pgxpool.Pool
	if !noopMode {
		p, err := NewDBPool(ctx, "host=host.docker.internal port=5432 dbname=betting_db user=betting_admin password=kiba")
		if err != nil {
			log.Fatalf("db connection failed: %v", err)
		}
		defer p.Close()
		pool = p
	}

	lis, err := net.Listen("tcp", listenAddr)
	if err != nil {
		log.Fatalf("listen failed: %v", err)
	}

	grpcServer := grpc.NewServer()
	pb.RegisterBettingServiceServer(grpcServer, NewBettingServer(pool))

	go func() {
		<-ctx.Done()
		log.Println("shutdown signal received, stopping gRPC server")
		grpcServer.GracefulStop()
	}()

	log.Printf("BettingService listening on %s", listenAddr)
	if err := grpcServer.Serve(lis); err != nil {
		log.Fatalf("serve failed: %v", err)
	}
}
