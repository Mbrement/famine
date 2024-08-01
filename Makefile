MANDATORY_DIR	=	sources
HEADERS_DIR		=	includes
OBJ_DIR			=	.objs

SRCS			=	$(shell find $(MANDATORY_DIR) -name "*.c")
SRCS_OC			=	$(shell find $(MANDATORY_DIR) -name "*.m")

OBJS			=	$(patsubst $(MANDATORY_DIR)%.c, $(OBJ_DIR)%.o, $(SRCS))

ifeq ($(shell uname), Darwin)
	OBJS += $(patsubst $(MANDATORY_DIR)%.m, $(OBJ_DIR)%.o, $(SRCS_OC))
	OCFLAGS	=	-framework Foundation
endif

HEADERS			=	$(shell find $(HEADERS_DIR) -name "*.h")

CC				=	gcc
ASM				=	nasm
RM				=	rm
CFLAGS			:=	-I$(HEADERS_DIR) -I$(MANDATORY_DIR) -g3 -O0 -Wall -Wextra -Werror

NAME			=	famine

GREEN			=	\033[1;32m
BLUE			=	\033[1;34m
RED				=	\033[1;31m
YELLOW			=	\033[1;33m
DEFAULT			=	\033[0m
UP				=	"\033[A"
CUT				=	"\033[K"

$(OBJ_DIR)/%.o: $(MANDATORY_DIR)/%.c $(HEADERS)
	@mkdir -p $(@D)
	@echo "$(YELLOW)Compiling [$<]$(DEFAULT)"
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf ${UP}${CUT}

$(OBJ_DIR)/%.o: $(MANDATORY_DIR)/%.m $(HEADERS)
	@mkdir -p $(@D)
	@echo "$(YELLOW)Compiling [$<]$(DEFAULT)"
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf ${UP}${CUT}

$(OBJ_DIR)/%.o: $(MANDATORY_DIR)/%.asm $(HEADERS)
	@mkdir -p $(@D)
	@echo "$(YELLOW)Compiling [$<]$(DEFAULT)"
	@$(ASM) $(ASMFLAGS) $< -o $@
	@printf ${UP}${CUT}

all: $(NAME)
	@$(MAKE) -sC woody-woodpacker
ifneq ($(shell uname), Darwin)
	./woody-woodpacker/woody_woodpacker $(NAME)
endif

$(NAME): $(OBJS) $(OBJS_ASM)
	@$(CC) $(CFLAGS) $(OCFLAGS) $^ -o $(NAME)
	@echo "$(GREEN)$(NAME) compiled!$(DEFAULT)"

unsafe:
	@$(MAKE) -s CFLAGS="$(CFLAGS) -D FM_SECURITY=0" all

clean:
	@echo "$(RED)Cleaning build folder$(DEFAULT)"
	-@$(RM) -r $(OBJ_DIR)

fclean: clean
	@echo "$(RED)Cleaning $(NAME)$(DEFAULT)"
	@$(RM) -f $(NAME)

kill:
	-@pkill $(NAME)

re: kill fclean all

.PHONY: all lib clean fclean re
