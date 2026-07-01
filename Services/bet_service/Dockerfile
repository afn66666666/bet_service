FROM golang:1.25

WORKDIR /app

RUN apt-get update && apt-get install -y protobuf-compiler && \
    go install google.golang.org/protobuf/cmd/protoc-gen-go@latest && \
    go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@latest

ENV PATH="/root/go/bin:${PATH}"

COPY . .

RUN mkdir -p proto && \
    protoc --go_out=proto --go_opt=paths=source_relative \
           --go-grpc_out=proto --go-grpc_opt=paths=source_relative \
           bet_service.proto

RUN go mod tidy && go build -o bet_service .

EXPOSE 50052

CMD ["./bet_service"]
