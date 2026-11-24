package com.indexforge.config;

import io.swagger.v3.oas.models.OpenAPI;
import io.swagger.v3.oas.models.info.Contact;
import io.swagger.v3.oas.models.info.Info;
import io.swagger.v3.oas.models.info.License;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

@Configuration
public class OpenApiConfig {

    @Bean
    public OpenAPI indexForgeOpenAPI() {
        return new OpenAPI()
            .info(new Info()
                .title("IndexForge Hybrid Search Platform API")
                .description("Java 21 Spring Boot Application Layer coordinating workflows, SQLite metadata, and high-performance C++20 Inverted Index Core via gRPC.")
                .version("2.0.0")
                .contact(new Contact().name("IndexForge Engineering Team"))
                .license(new License().name("Apache 2.0")));
    }
}
