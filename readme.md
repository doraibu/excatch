# Excatch: Runtime de Exceções para C (x86_64)

Esta biblioteca implementa um sistema de tratamento de exceções, que na verdade se comporta como uma versão um pouco trabalhada de um setjmp/longjmp, e RAII (Resource Acquisition Is Initialization) em C puro, utilizando o modelo de registro dinâmico de contexto e varredura de pilha.

## 1. Captura e Restauração de Contexto

O mecanismo utiliza Assembly para manipular o fluxo de execução sem as abstrações do compilador.

- **Ponto de Salvamento (`try`):** O macro armazena o estado atual dos registradores voláteis e não-voláteis (`RBX`, `RSP`, `RBP`, `R12-R15`) em uma estrutura global `_exc_arena`.
- **Endereço de Retorno:** A instrução `leaq 1f(%%rip), %%rax` captura o endereço de instrução (RIP) relativo à label de salto, permitindo que o `throw` retorne exatamente para o ponto logo após a definição do bloco de salvamento.
- **Desvio de Fluxo (`throw`):** A função `exc_throw` restaura os registradores salvos e executa um salto absoluto (`jmp *%0`) para o RIP armazenado, simulando o retorno de uma chamada de função com um código de erro no registrador `EAX`.

## 2. RAII e Gerenciamento de Escopo

A limpeza automática de recursos baseia-se em extensões de escopo do GCC/Clang e marcação de ponteiros.

- **Atributo de Cleanup:** O macro `RAII_VAR` utiliza `__attribute__((cleanup(função)))`, instruindo o compilador a inserir uma chamada ao destrutor assim que a variável sai do escopo léxico.
- **Pointer Tagging:** Ponteiros gerenciados via `raii_malloc` sofrem uma operação de OR binário (`| 0x1`) no bit menos significativo. Isso permite distinguir, durante a varredura, quais valores na pilha representam recursos protegidos sem a necessidade de uma tabela externa.

## 3. Stack Unwinding Manual

Diferente do C++, que utiliza tabelas estáticas (`.eh_frame`), esta implementação realiza o desempilhamento de forma ativa durante o erro.

- **Varredura Linear:** A função `_unwind_stack` percorre a memória entre o `RSP` atual (momento do erro) e o `target_rsp` (limite do bloco `try` correspondente).
- **Identificação de Recursos:** Durante a iteração, cada palavra de memória na pilha é testada pelo macro `RAII_IS_TAGGED`. Se um ponteiro marcado for encontrado, o sistema invoca `raii_free`, que remove a tag, localiza o cabeçalho `_raii_hdr` (contendo o ponteiro original e o destrutor) e libera o recurso.

## Problemas

- **Nesting:** O nest de blocos `try` é impossível por agora por colisão de hashes e sobrescrita de slots
- **Riscos:** É possível que algum inteiro (`int`) infelizmente termine com o ultimo bit 0x1, confundindo o Stack Unwinding e fazendo um jump e tentativa de destruição em uma região não previsível e com certeza fora dos limites da arena, o que resultará em uma Segmentation Violation

## Avisos

[Um aviso ao leitor](wtoreader.md)

## Requisitos Técnicos

- **ISA:** x86_64 (devido à manipulação direta de registradores de 64 bits).
- **Compilador:** Compatível com extensões GNU (GCC/Clang) para suporte ao atributo `cleanup`.
