# ZeusClient

## Introduction

This is the client utility of my Wifinanny program. The Wifinanny was an idea
I had when completing my first Master's at WGU. The idea is to implement
a transparent proxy on a wifi router, which captures the URL requests. This
is easily accomplished using Squid. Using this client as a squidhelper, we
can submit the url to a centralized database daemon (zeusd), which will then
check to see if the client is allowed to visit the url.

## Installation

Installation is very simple. Just chose the applicable make file and compile.
I used a Ralink MIPS SoC, this required cross-compilation.

## License

Ignore any headers regarding proprietary blah-blah. It's hereby released
as GPL. Cluecentral Inc was dissolved, and I was its president.
