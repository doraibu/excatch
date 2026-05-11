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

Essa biblioteca não é séria, não se trata de um projeto real a ser mantido, ela provavelmente tende a ser abandonada e arquivada quando o objetivo de implementação for cumprido.
Dito isso, não utilize qualquer uma das ferramentas estabelecidas nesse repositório em algum projeto real, se a necessidade de Exceptions for real, utilize uma linguagem que forneça suporte a essa prática como C++ ou Java ao invés de tentar implementar algo assim em C, isso é somente um teste dos limites da linguagem (e da criatividade :D).
O uso do termo "Stack Unwinding" é meio mentiroso se considerado a definição de outras linguagens como C++, afinal aqui não existe a pilha de símbolos que carrega exatamente cada objeto e consegue chamar o destrutor individualmente em cada um ao ser dado o erro, o erro aqui é propagado "globalmente", a primeira exception a ser lançada vai varrer a stack inteira liberando tudo termiando em 0x1, é mais semelhante a um garbage collector que é ativado no primeiro erro no seu funcionamento interno, porém como o efeito é a limpeza efetiva da Stack, podemos chamar de Unwind mesmo não sendo o mesmo funcionamento ou implementação, porque ele efetivamente "volta" a Stack para um estado anterior.
RAII também não é exatamente "real" nesse contexto, é uma enganação implementada por um atributo especifico da GNU que força a liberação/destruição por uma função de limpeza, porém é uma tremenda gambiarra principalmente no contexto implementado, entretanto como o funcionamento e efeito final são semelhantes ao RAII original, podemos chamar por esse termo.

Um dos fatores que impede a implementação real de Exceptions é que o compilador ao lançar uma exception faz dezenas de checks e depois procura o catch exato pra aquela exception, em outras palavras e uma comparação ruim, o compilador busca o catch na tabela como um kernel busca o socket de rede na lista de sockets, a diferença é que não da pra mandar um compilador que não tem essa capacidade (GCC) fazer isso, diferente que um kernel que facilmente le uma tabela de sockets. Tecnicamente falando, é quase impossível implementar RAII e Unwinding real em C, e antes que estejam lendo isso e venham me falar das extensões GNU ou como escrever num binário ELF para montar as tabelas, eu sei como isso funciona, a questão é que o RAII de verdade e o Stack Unwinding NÃO são coisas implementaveis por software, mesmo que você implemente escrevendo direto uma tabela no ELF que contenha exatamente o que o C++ contém, você teria que fazer uma lógica que forçaria o compilador a pular pra ela toda vez que lança uma exception, o que não é exatamente como funciona por conta da falta de destrutores nativos e o fato do C não ter um contrato nativo para a busca por `catch` conforme exceptions são lançadas, o que também configura implementar a RTTI (Run-Time Type Information) do C++ por inteiro, o que é basicamente impossível considerando a complexidade e os contratos especificos que o compilador necessita cumprir, efetivar isso não seria implementar try/catch em C, mas sim transformar C em C++, o ponto é: você até pode simular o comportamento dessas coisas, mas a implementação REAL e o funcionamento real são coisas suportadas pelo compilador, uma comparação burra é tentar simular AVX numa CPU só pra tentar ver se da performance, se a CPU não suporta o AVX você só ta fazendo uma boa gambiarra, pra implementar RAII e Unwinding no C você teria que, literalmente, forkar o GCC e implementar você mesmo, o que é outra classe de problema.

## Requisitos Técnicos

- **ISA:** x86_64 (devido à manipulação direta de registradores de 64 bits).
- **Compilador:** Compatível com extensões GNU (GCC/Clang) para suporte ao atributo `cleanup`.
