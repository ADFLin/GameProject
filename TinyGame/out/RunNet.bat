@echo off
start "Server" TinyGame.exe -AutoNet -Server -Output ../server_log.txt
timeout 2
start "Client" TinyGame.exe -AutoNet -Client -Output ../client_log.txt