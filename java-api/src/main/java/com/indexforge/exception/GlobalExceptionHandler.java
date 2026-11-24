package com.indexforge.exception;

import io.grpc.Status;
import io.grpc.StatusRuntimeException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import java.time.LocalDateTime;
import java.util.HashMap;
import java.util.Map;

@RestControllerAdvice
public class GlobalExceptionHandler {

    private static final Logger log = LoggerFactory.getLogger(GlobalExceptionHandler.class);

    @ExceptionHandler(EngineUnavailableException.class)
    public ResponseEntity<Map<String, Object>> handleEngineUnavailable(EngineUnavailableException e) {
        log.warn("C++ Engine unavailable: {}", e.getMessage());
        return buildErrorResponse(HttpStatus.SERVICE_UNAVAILABLE, "C++ Search Engine is currently unreachable. Please check gRPC service.", e.getMessage());
    }

    @ExceptionHandler(InvalidQueryException.class)
    public ResponseEntity<Map<String, Object>> handleInvalidQuery(InvalidQueryException e) {
        return buildErrorResponse(HttpStatus.BAD_REQUEST, "Invalid search query syntax", e.getMessage());
    }

    @ExceptionHandler(StatusRuntimeException.class)
    public ResponseEntity<Map<String, Object>> handleGrpcException(StatusRuntimeException e) {
        Status.Code code = e.getStatus().getCode();
        log.error("gRPC error [{}]: {}", code, e.getMessage());

        HttpStatus httpStatus = switch (code) {
            case UNAVAILABLE -> HttpStatus.SERVICE_UNAVAILABLE;
            case DEADLINE_EXCEEDED -> HttpStatus.GATEWAY_TIMEOUT;
            case INVALID_ARGUMENT -> HttpStatus.BAD_REQUEST;
            case NOT_FOUND -> HttpStatus.NOT_FOUND;
            case PERMISSION_DENIED -> HttpStatus.FORBIDDEN;
            default -> HttpStatus.INTERNAL_SERVER_ERROR;
        };

        return buildErrorResponse(httpStatus, "gRPC Engine Error (" + code + ")", e.getStatus().getDescription());
    }

    @ExceptionHandler(MethodArgumentNotValidException.class)
    public ResponseEntity<Map<String, Object>> handleValidationErrors(MethodArgumentNotValidException e) {
        String msg = e.getBindingResult().getFieldErrors().stream()
                .findFirst()
                .map(f -> f.getField() + ": " + f.getDefaultMessage())
                .orElse("Validation error");
        return buildErrorResponse(HttpStatus.BAD_REQUEST, "Validation Failure", msg);
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity<Map<String, Object>> handleGenericException(Exception e) {
        log.error("Unhandled server exception: {}", e.getMessage(), e);
        return buildErrorResponse(HttpStatus.INTERNAL_SERVER_ERROR, "Internal Application Error", e.getMessage());
    }

    private ResponseEntity<Map<String, Object>> buildErrorResponse(HttpStatus status, String error, String details) {
        Map<String, Object> body = new HashMap<>();
        body.put("timestamp", LocalDateTime.now().toString());
        body.put("status", status.value());
        body.put("error", error);
        body.put("details", details != null ? details : "");
        return ResponseEntity.status(status).body(body);
    }
}
