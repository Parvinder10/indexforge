package com.indexforge.config;

import com.indexforge.grpc.SearchEngineServiceGrpc;
import io.grpc.ManagedChannel;
import io.grpc.ManagedChannelBuilder;
import jakarta.annotation.PreDestroy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import java.util.concurrent.TimeUnit;

@Configuration
public class GrpcClientConfig {

    private static final Logger log = LoggerFactory.getLogger(GrpcClientConfig.class);

    @Value("${indexforge.grpc.host:localhost}")
    private String grpcHost;

    @Value("${indexforge.grpc.port:50051}")
    private int grpcPort;

    private ManagedChannel channel;

    @Bean
    public ManagedChannel managedChannel() {
        log.info("[*] Initializing gRPC channel to C++ Search Engine at {}:{}", grpcHost, grpcPort);
        this.channel = ManagedChannelBuilder.forAddress(grpcHost, grpcPort)
                .usePlaintext()
                .maxInboundMessageSize(64 * 1024 * 1024)
                .keepAliveTime(30, TimeUnit.SECONDS)
                .keepAliveTimeout(10, TimeUnit.SECONDS)
                .build();
        return this.channel;
    }

    @Bean
    public SearchEngineServiceGrpc.SearchEngineServiceBlockingStub searchEngineBlockingStub(ManagedChannel channel) {
        return SearchEngineServiceGrpc.newBlockingStub(channel);
    }

    @PreDestroy
    public void shutdown() {
        if (channel != null && !channel.isShutdown()) {
            log.info("[*] Shutting down gRPC channel...");
            try {
                channel.shutdown().awaitTermination(3, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                channel.shutdownNow();
            }
        }
    }
}
