package main

import (
	"context"

	"github.com/jackc/pgx/v5/pgxpool"
)

func NewDBPool(ctx context.Context, connString string) (*pgxpool.Pool, error) {
	return pgxpool.New(ctx, connString)
}
