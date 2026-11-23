# National Intelligent Transportation Systems (ITS) Reference Architecture
**Agency:** Department of Transportation (DOT), Federal Highway Administration  
**Date:** 2026-05-18  
**Document Type:** tech_doc  
**Publication:** DOT-FHWA-ITS-2026  

## 1. System Overview
The National ITS Architecture provides a common framework for planning, defining, and integrating intelligent transportation systems. Modern surface transportation networks increasingly rely on Vehicle-to-Everything (V2X) wireless communications, roadside edge processing units, and real-time traffic signal optimization algorithms.

## 2. Technical Architecture & Protocols
- **Data Exchange Interfaces:** Transportation management centers interface with vehicle onboard units using IEEE 1609 wireless access in vehicular environments (WAVE) protocols and SAE J2735 message sets.
- **Latency Constraints:** Critical safety messages, such as forward collision warnings and emergency vehicle preemption requests, require end-to-end network transmission latency under 20 milliseconds.
- **Cybersecurity & Trust Management:** Vehicle and infrastructure telemetry must be authenticated through the Security Credential Management System (SCMS) utilizing elliptic curve digital signatures (ECDSA) to prevent spoofing and unauthorized signal manipulation.
