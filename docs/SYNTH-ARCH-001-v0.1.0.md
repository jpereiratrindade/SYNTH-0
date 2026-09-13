# SYNTH-ARCH-001 — Bootstrap de um ecossistema sintético declarativo

```context-metadata+json
{
  "document": {
    "id": "SYNTH-ARCH-001",
    "version": "0.1.0",
    "status": "direction-candidate",
    "title": "Bootstrap de um ecossistema sintético declarativo",
    "ecosystem_codename": "SYNTH-0",
    "created_at": "2026-09-13T09:49:00-03:00",
    "language": "pt-BR"
  },
  "purpose": {
    "kind": "architectural-direction",
    "intent": "Criar do zero um ecossistema sintético, deliberadamente pequeno, para experimentar independência entre sistemas, relações declarativas, reconciliação, coerência sob distúrbio, eficiência de recursos e uma experiência web única."
  },
  "provenance": {
    "emergence": {
      "when": "2026-09-13",
      "how": "A ideia surgiu durante uma discussão sobre o Windows Registry e sua comparação com a organização declarativa típica de sistemas Unix e, especialmente, com a noção de configuração declarativa e convergência encontrada em NixOS. A discussão levou à hipótese de que contratos, autoridade, estado observado e reconciliação poderiam constituir uma arquitetura geral de ecossistemas de sistemas independentes.",
      "turning_point": "Em vez de aplicar imediatamente a hipótese a um ecossistema existente, decidiu-se construir um ecossistema sintético do zero para que os conceitos pudessem ser testados desde a primeira linha de código, sem herdar decisões anteriores."
    },
    "influences": [
      "filosofia Unix: composição, arquivos legíveis, responsabilidades estreitas",
      "NixOS: declaração, realização e convergência",
      "control loops/reconciliation: desired state versus observed state",
      "systemd: supervisão de processos em vez de supervisor próprio",
      "arquitetura orientada a contratos e versionamento explícito"
    ]
  },
  "principles": [
    "independência operacional e evolutiva dos sistemas integrantes",
    "relações explícitas, tipadas, versionadas e observáveis",
    "declaração separada do estado factual",
    "nenhuma segunda fonte de verdade",
    "sempre pronto, mas sempre incompleto",
    "coerência durante reconfiguração e degradação",
    "otimização orientada por medição, com filas e memória limitadas",
    "experiência web única em vez de coleção de painéis administrativos",
    "sem reinventar supervisores, protocolos, hashing, build systems ou formatos quando padrões maduros já existem"
  ],
  "review": {
    "passes": 5,
    "lenses": [
      "autoridade, contratos e ausência de ambiguidade",
      "falhas, distúrbios, convergência e recuperação",
      "independência dos sistemas e capacidade de separação futura",
      "uso de CPU/memória e controle de backpressure",
      "experiência web, testabilidade e bootstrap em um único patch"
    ]
  }
}
```

> **Natureza deste documento:** direção arquitetural experimental.
> Não é uma especificação congelada. Ele define invariantes suficientes para o primeiro patch e deixa deliberadamente abertas as partes que só devem ser decididas depois de observação empírica.

---

## 1. Hipótese central

Construir um ecossistema não como um programa único, nem como um conjunto de microsserviços ligados por convenção informal, mas como:

\[
\boxed{
E = S + R + D + O + \Gamma
}
\]

onde:

- \(S\) = sistemas independentes;
- \(R\) = relações declaradas entre eles;
- \(D\) = declarações de estado desejado;
- \(O\) = observações factuais;
- \(\Gamma\) = mecanismo de reconciliação/convergência.

O ecossistema não deve possuir um “Registry” central no sentido do Windows.

A alternativa proposta é um **espaço declarativo distribuído**, legível, versionado e auditável, no qual cada sistema mantém sua própria autoridade e as relações são declaradas explicitamente.

A regra é:

\[
\text{declaração} \neq \text{observação}
\]

e a operação normal do ecossistema é:

\[
\text{declarar}
\rightarrow
\text{observar}
\rightarrow
\text{comparar}
\rightarrow
\text{planejar}
\rightarrow
\text{agir}
\rightarrow
\text{observar novamente}
\rightarrow
\text{verificar}
\]

---

# 2. Codinome do experimento

## SYNTH-0

O nome é propositalmente provisório.

SYNTH-0 não é produto. É um **organismo arquitetural experimental mínimo**.

O primeiro objetivo não é resolver um domínio real.

O objetivo é tornar observáveis as propriedades arquiteturais que queremos testar:

- autonomia;
- autoridade;
- relação;
- convergência;
- degradação;
- reconfiguração;
- evidência;
- custo computacional;
- experiência humana.

---

# 3. Os três sistemas sintéticos

Os sistemas não devem compartilhar estado interno, banco, código de domínio ou arquivos privados.

Eles podem conhecer apenas contratos públicos.

## 3.1 `pulso`

### Responsabilidade

Produzir fatos sintéticos temporais.

Exemplos:

- valor;
- timestamp;
- qualidade;
- sequência;
- taxa de emissão.

`pulso` é deliberadamente simples porque funciona como fonte factual do experimento.

### Autoridade

```text
authority: synth.pulso
```

### Possíveis capacidades

```text
signal.observe
signal.stream
signal.rate
```

### Estado interno

Próprio.

Nenhum outro sistema pode escrever diretamente nele.

---

## 3.2 `trama`

### Responsabilidade

Relacionar fatos e produzir estrutura derivada.

Recebe referências para fatos de `pulso` e constrói:

- janelas;
- agrupamentos;
- relações;
- padrões;
- derivações.

### Autoridade

```text
authority: synth.trama
```

### Possíveis capacidades

```text
relation.build
pattern.observe
pattern.stream
```

`trama` nunca “possui” o fato de `pulso`.

Ela possui apenas sua própria interpretação/derivação.

---

## 3.3 `lente`

### Responsabilidade

Construir projeções e intenções.

`lente` transforma resultados de `trama` em perspectivas utilizáveis e pode emitir uma **intenção**, por exemplo:

> reduzir a taxa de emissão de `pulso` quando determinado padrão ocorrer.

A intenção não autoriza `lente` a editar `pulso`.

Ela produz um objeto explícito que poderá ser aceito ou rejeitado pelo mecanismo de reconciliação conforme contrato.

### Autoridade

```text
authority: synth.lente
```

### Possíveis capacidades

```text
projection.build
projection.observe
intent.propose
```

---

# 4. A relação entre os três sistemas

O experimento deve formar um pequeno ciclo:

```text
          fatos
 PULSO ─────────────► TRAMA
   ▲                    │
   │                    │ padrões
   │                    ▼
   └──────── intenção ─ LENTE
```

Isso é intencional.

Um pipeline puramente linear seria simples demais para testar coerência sistêmica.

A existência do ciclo força o ecossistema a lidar com:

- causalidade operacional;
- atraso;
- versões;
- indisponibilidade;
- decisões rejeitadas;
- dependência degradada;
- reconfiguração.

---

# 5. O “registro” do ecossistema

Não haverá daemon de registry.

Não haverá banco central de configuração.

Não haverá API central proprietária de todas as identidades.

O “registro” é um conjunto de declarações versionadas.

Estrutura inicial:

```text
declarations/
├── ecosystem.toml
├── systems/
│   ├── pulso.toml
│   ├── trama.toml
│   └── lente.toml
└── relations/
    ├── pulso-to-trama.toml
    ├── trama-to-lente.toml
    └── lente-to-pulso.toml
```

Os arquivos são humanos, versionáveis por Git e validáveis por máquina.

---

# 6. Regra de autoridade

Toda entidade pública deve possuir:

```text
authority
kind
id
version
```

Uma referência mínima será:

```toml
authority = "synth.pulso"
kind = "signal"
id = "primary"
version = "1"
```

Nenhum sistema pode assumir autoridade sobre objetos pertencentes a outro sistema.

Uma relação aponta para referências.

Nunca copia autoridade.

---

# 7. Declaração, projeção e testemunha

Devem existir três classes explicitamente diferentes de artefatos.

## 7.1 Declaração

O que queremos.

```text
declarations/
```

Versionado por Git.

Editado por humanos ou ferramentas autorizadas.

---

## 7.2 Projeção

O que foi derivado das declarações para ser executado.

```text
.state/generated/
```

Nunca editado manualmente.

Pode ser apagado e reconstruído.

---

## 7.3 Testemunha factual

O que foi realmente observado.

```text
.evidence/
```

Exemplos:

```text
process.json
endpoint.json
contract.json
resources.json
relation.json
```

A testemunha não declara intenção.

Ela relata fato.

---

# 8. Reconciliação

O reconciliador não deve perguntar simplesmente:

> “o processo está rodando?”

Ele deve comparar declaração, projeção e estado factual.

Resultado mínimo:

```text
KEEP
REPAIR
RECONFIGURE
DEGRADED
BLOCKED
```

### KEEP

Declaração e fato são coerentes.

### REPAIR

A declaração continua correta, mas o runtime divergiu.

Exemplo:

```text
processo morreu
```

### RECONFIGURE

A declaração mudou e uma nova realização é necessária.

### DEGRADED

O sistema consegue continuar operando, porém uma relação não está plenamente satisfeita.

### BLOCKED

Prosseguir violaria uma invariante obrigatória.

---

# 9. Pós-convergência obrigatória

Uma ação não é considerada concluída porque retornou exit code zero.

Depois de qualquer modificação:

```text
OBSERVE AGAIN
PLAN AGAIN
```

A convergência só está demonstrada quando o novo plano é:

```text
KEEP
```

ou quando existe uma degradação explicitamente permitida pelo contrato.

---

# 10. “Sempre pronto, mas sempre incompleto”

Essa frase se torna uma propriedade formal do projeto.

## Pronto

Significa:

> Todas as capacidades declaradas como ativas satisfazem seus contratos mínimos.

## Completo

Não é requisito.

Novas capacidades podem surgir indefinidamente.

Logo:

\[
readiness \neq completeness
\]

Um recurso inexistente não quebra o ecossistema se não tiver sido declarado como obrigatório.

O sistema nunca deve fingir que algo existe.

Estados possíveis de capacidade:

```text
absent
experimental
ready
degraded
```

---

# 11. Coerência sob distúrbio

A propriedade central a ser testada será:

> **O ecossistema deve conservar suas invariantes conhecidas enquanto se reconfigura diante de um distúrbio.**

Não significa permanecer sempre totalmente funcional.

Significa nunca entrar silenciosamente em um estado cuja interpretação seja desconhecida.

Formalmente:

\[
distúrbio
\rightarrow
estado\ observado
\rightarrow
estado\ interpretável
\rightarrow
ação\ justificável
\rightarrow
nova\ coerência
\]

---

# 12. Distúrbios obrigatórios do experimento

O bootstrap já deve incluir um injetor de falhas de laboratório.

Casos mínimos:

1. matar `pulso`;
2. matar `trama`;
3. matar `lente`;
4. alterar uma declaração;
5. produzir incompatibilidade de versão de contrato;
6. atrasar artificialmente uma relação;
7. saturar uma fila;
8. limitar memória;
9. reiniciar um sistema durante fluxo ativo;
10. apresentar testemunha factual antiga.

Cada caso precisa demonstrar uma transição observável.

Exemplo:

```text
HEALTHY
→ DISTURBED
→ DEGRADED
→ REPAIRING
→ CONVERGING
→ HEALTHY
```

---

# 13. Supervisão: não reinventar

O ecossistema não deve implementar um supervisor de processos próprio.

No Linux real:

```text
systemd --user
```

será o mecanismo de supervisão.

O código do ecossistema permanece responsável por:

- declaração;
- projeção;
- observação;
- comparação;
- decisão.

O systemd permanece responsável por:

- iniciar;
- parar;
- reiniciar;
- supervisionar processos.

Para testes e CI pode existir um backend efêmero de processos, mas apenas como harness.

---

# 14. Transporte e protocolos

No primeiro patch:

### Externo

```text
HTTP + JSON
```

### Atualizações para navegador

```text
Server-Sent Events (SSE)
```

Motivos:

- padrão web;
- simples;
- debuggável;
- reconexão nativa;
- não exige protocolo proprietário.

### IPC local

Preferencialmente:

```text
HTTP sobre Unix Domain Socket
```

quando possível.

Nenhum protocolo binário próprio no primeiro ciclo.

Se medições futuras mostrarem necessidade real, a troca será feita preservando o contrato semântico.

---

# 15. Serialização

## Declarações humanas

```text
TOML
```

## API e evidências

```text
JSON
```

## Normalização para identidade

As declarações devem possuir representação canônica antes do hash.

Digest inicial:

```text
SHA-256
```

Não criar algoritmo próprio.

O digest permitirá responder:

> “esta projeção foi produzida exatamente por qual declaração?”

---

# 16. C++26

Todo componente executável será C++26.

Regra:

```cmake
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

Mas o projeto não deve usar novidade de linguagem apenas porque ela existe.

C++26 deve servir a:

- ownership claro;
- composição;
- segurança;
- desempenho;
- expressividade.

---

# 17. Estrutura de cada sistema

Cada sistema deve ser autocontido.

```text
systems/pulso/
├── CMakeLists.txt
├── include/
├── src/
├── tests/
├── contracts/
├── web/
└── README.md
```

E o mesmo para:

```text
trama
lente
```

Proibição arquitetural:

```text
#include "../../trama/..."
```

ou qualquer equivalente.

Nenhum sistema pode incluir código privado de outro.

---

# 18. Separação futura em repositórios

O primeiro patch pode viver em um monorepo para tornar o experimento barato.

Porém as fronteiras devem permitir:

```text
systems/pulso  -> repositório independente
systems/trama  -> repositório independente
systems/lente  -> repositório independente
```

sem alteração de contrato.

Portanto:

> o monorepo é conveniência física, não acoplamento arquitetural.

---

# 19. Build e dependências

Base:

```text
CMake
Ninja
CMakePresets.json
CTest
```

Dependências externas devem ser poucas, explícitas e fixadas por versão.

Preferência:

- bibliotecas maduras;
- padrões existentes;
- dependências justificadas.

Não criar:

- framework HTTP próprio;
- logger próprio;
- parser TOML próprio;
- parser JSON próprio;
- sistema de hash próprio;
- test framework próprio.

---

# 20. Otimização CPU ↔ memória

O princípio não será:

> “usar a menor memória possível”.

Nem:

> “usar toda a CPU disponível”.

Será:

\[
\boxed{
minimizar\ custo\ total\ sob\ restrições\ explícitas
}
\]

Cada sistema declara orçamento:

```toml
[resources]
max_memory_mb = 128
max_queue_items = 4096
target_cpu_percent = 25
```

O reconciliador mede o estado factual.

---

# 21. Regras de memória

Desde o bootstrap:

- nenhuma fila sem limite;
- nenhum cache sem política de expiração;
- nenhuma acumulação histórica infinita;
- buffers circulares onde a história completa não for requisito;
- backpressure explícito;
- ownership explícito;
- cópias evitadas no hot path;
- `std::pmr` somente onde medição justificar arenas;
- estruturas compactas em fluxos frequentes;
- persistência separada da memória de trabalho.

---

# 22. Regra de otimização

Não otimizar por intuição.

Fluxo:

```text
medir
→ localizar custo
→ formular hipótese
→ alterar
→ medir novamente
```

Métricas mínimas:

```text
RSS
CPU time
latência p50/p95/p99
queue depth
throughput
restart time
convergence time
```

A métrica mais importante do ecossistema será:

```text
time_to_coherence
```

Tempo entre:

```text
distúrbio detectado
```

e:

```text
nova condição coerente verificada
```

---

# 23. Interface web: não será um dashboard

A página inicial não deve mostrar três cartões verdes.

A experiência deverá tornar visível:

```text
sistema
+
relação
+
declaração
+
estado factual
+
evidência
+
transformação no tempo
```

Nome provisório da experiência:

# Living Topology

---

# 24. Gramática visual

Tela principal:

```text
           PULSO
          ╱     ╲
         ╱       ╲
     intenção    fatos
       ╱           ╲
    LENTE ─────── TRAMA
          padrões
```

Cada nó mostra apenas:

- identidade;
- capacidade dominante;
- estado.

Cada aresta representa uma relação real.

Não uma linha decorativa.

---

# 25. Distúrbio como experiência

O usuário poderá abrir:

```text
Disturbance Lab
```

e executar:

```text
Kill Pulso
Add latency
Break contract version
Reduce memory
Change declaration
Freeze Trama
```

A interface deve mostrar ao vivo:

```text
distúrbio
↓
observação
↓
novo plano
↓
ação
↓
re-observação
↓
convergência
```

Assim, a arquitetura se torna experimentável.

---

# 26. Painel de evidência

Ao selecionar qualquer nó ou relação:

```text
Declared
Observed
Delta
Evidence
History
```

A interface não deve exigir que a pessoa leia logs para compreender o sistema.

Logs continuam disponíveis.

Mas são evidência de último nível.

---

# 27. Tecnologia web

Primeira versão:

```text
TypeScript
Web Components
SVG
CSS
Vite
```

Sem framework de aplicação pesado inicialmente.

SVG permite:

- relações navegáveis;
- semântica;
- acessibilidade;
- inspeção;
- animação controlada.

Requisitos:

- teclado;
- foco visível;
- reduced-motion;
- alto contraste;
- layout responsivo;
- sem dependência de canvas para informação essencial.

---

# 28. A árvore inicial do projeto

```text
SYNTH-0/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── declarations/
│   ├── ecosystem.toml
│   ├── systems/
│   │   ├── pulso.toml
│   │   ├── trama.toml
│   │   └── lente.toml
│   └── relations/
│       ├── pulso-to-trama.toml
│       ├── trama-to-lente.toml
│       └── lente-to-pulso.toml
├── schemas/
├── controller/
│   ├── include/
│   ├── src/
│   └── tests/
├── systems/
│   ├── pulso/
│   ├── trama/
│   └── lente/
├── web/
├── tests/
│   ├── contract/
│   ├── integration/
│   ├── resilience/
│   └── resource/
├── packaging/
│   └── systemd/
├── docs/
└── bin/
```

---

# 29. CLI única de ecossistema

```text
synth validate
synth observe
synth plan
synth apply
synth verify
synth status
synth evidence
synth disturb
synth run
```

Comando canônico:

```bash
./bin/synth run --target dev
```

Sem argumentos mágicos escondidos.

---

# 30. Semântica de `run`

```text
VALIDATE
↓
OBSERVE
↓
PLAN
↓
APPLY
↓
OBSERVE AGAIN
↓
PLAN AGAIN
↓
VERIFY
↓
READY
```

Uma segunda execução sem mudança deve resultar em:

```text
NO_OP
```

Isso será um teste obrigatório.

---

# 31. O primeiro patch

O patch inaugural deve levantar **tudo que é necessário para provar a arquitetura**, e nada além disso.

Ele deve entregar:

### Infraestrutura

- CMake;
- Ninja;
- presets;
- testes;
- CI.

### Contratos

- schema de sistema;
- schema de relação;
- schema de evidência;
- schema de plano.

### Controller

- validate;
- observe;
- plan;
- apply;
- verify.

### Sistemas

- `pulso`;
- `trama`;
- `lente`.

### Runtime

- modo local;
- systemd user adapter.

### Web

- Living Topology;
- SSE;
- painel de evidência;
- Disturbance Lab mínimo.

### Testes

- contrato;
- integração;
- idempotência;
- kill/recovery;
- contract mismatch;
- queue saturation;
- resource budget.

---

# 32. O que deliberadamente NÃO entra no primeiro patch

- Kubernetes;
- service mesh;
- Kafka;
- banco distribuído;
- consenso;
- Raft;
- event sourcing global;
- GraphQL;
- protocolo próprio;
- DSL própria;
- framework web próprio;
- plugin system;
- banco central de configuração;
- autenticação complexa;
- multi-tenant;
- microkernel abstrato;
- otimizações sem benchmark.

A omissão é uma decisão arquitetural.

---

# 33. Critério de sucesso do bootstrap

O primeiro experimento estará aprovado quando for possível demonstrar:

```text
1. três sistemas sob autoridades diferentes;
2. três relações declarativas;
3. inicialização completa;
4. testemunha factual de cada relação;
5. segunda execução NO_OP;
6. morte de um sistema;
7. detecção factual;
8. degradação coerente;
9. reparo;
10. re-observação;
11. retorno a KEEP;
12. visualização integral dessa sequência na interface.
```

Se isso acontecer, o ecossistema já terá demonstrado seu princípio essencial.

---

# 34. Critérios de independência

Após o bootstrap, deve ser possível trabalhar exclusivamente em:

```text
systems/pulso
```

sem recompilar `trama` ou `lente`, salvo quando um contrato público mudar.

O mesmo vale para os outros sistemas.

Mudança interna:

```text
não propaga
```

Mudança de contrato:

```text
propaga explicitamente
```

---

# 35. Versões de contrato

Contratos terão versão própria.

Exemplo:

```text
synth.contract.signal/1.0.0
```

A versão do binário não substitui a versão do contrato.

Exemplo:

```text
pulso binary: 0.8.2
signal contract: 1.1.0
```

São dimensões diferentes.

---

# 36. Compatibilidade

Relações declaram o intervalo aceito.

Exemplo:

```toml
[requires]
contract = "synth.contract.signal"
version = ">=1.0.0 <2.0.0"
```

Uma incompatibilidade precisa ser visível como estado.

Nunca deve se tornar undefined behavior sistêmico.

---

# 37. Segurança epistemológica

Toda informação exibida deverá ser classificável como:

```text
DECLARED
DERIVED
OBSERVED
INFERRED
```

No primeiro patch, preferir somente:

```text
DECLARED
DERIVED
OBSERVED
```

`INFERRED` fica reservado para quando realmente existir inferência.

Isso evita que uma projeção seja apresentada como fato.

---

# 38. Invariantes fundadoras

## I-01 — Autoridade única

Um objeto possui exatamente uma autoridade.

## I-02 — Nenhuma escrita transfronteira

Um sistema não escreve estado privado de outro.

## I-03 — Relações são explícitas

Dependências implícitas são defeito.

## I-04 — Declaração não é fato

Toda realização precisa ser observada.

## I-05 — Convergência precisa ser demonstrada

Aplicar não implica convergir.

## I-06 — Filas são limitadas

Não existe buffer infinito.

## I-07 — Falha precisa ser interpretável

Nenhum estado silenciosamente desconhecido.

## I-08 — Ausência pode ser válida

Incompletude não implica indisponibilidade.

## I-09 — Evidência acompanha decisão

Toda decisão de reconciliação deve apontar para evidência.

## I-10 — UI não inventa estado

Tudo que aparece como factual na interface deve possuir witness.

---

# 39. Estratégia de desenvolvimento após o patch inaugural

Depois do bootstrap, os três sistemas deixam de evoluir em sincronia obrigatória.

Cada um passa a possuir:

```text
roadmap próprio
release própria
contratos próprios
testes próprios
histórico próprio
```

O ecossistema integra versões compatíveis.

Isto transforma:

```text
monorepo inicial
```

em:

```text
ecossistema de sistemas
```

sem exigir uma reescrita posterior.

---

# 40. Cinco revisões aplicadas ao plano

## Revisão 1 — Autoridade

Resultado:

- não existe registro central;
- cada objeto tem autoridade;
- relações usam referências;
- declaração e observação permanecem distintas.

## Revisão 2 — Distúrbio

Resultado:

- falha é estado de primeira classe;
- recuperação exige re-observação;
- coerência degradada é permitida quando conhecida;
- systemd assume supervisão real.

## Revisão 3 — Independência

Resultado:

- nenhum include cruzado;
- nenhum banco compartilhado;
- contratos são fronteira;
- monorepo não se torna dependência arquitetural.

## Revisão 4 — CPU/memória

Resultado:

- filas limitadas;
- backpressure;
- orçamento declarativo;
- otimização somente após medição;
- `time_to_coherence` introduzido como métrica sistêmica.

## Revisão 5 — Experiência e bootstrap

Resultado:

- uma única topologia viva;
- distúrbios visíveis;
- evidência acessível;
- primeiro patch prova a tese completa;
- nenhum componente infraestrutural importante é reinventado.

---

# 41. Síntese arquitetural

O experimento não procura responder:

> “como fazer três serviços conversarem?”

Essa pergunta é pequena demais.

A pergunta é:

> **Como sistemas autônomos podem formar um ecossistema reconhecível, observável e coerente sem perder sua independência — inclusive enquanto o próprio ecossistema está mudando?**

A proposta inicial é:

\[
\boxed{
\text{Autonomia}
+
\text{Contratos}
+
\text{Autoridade}
+
\text{Observação}
+
\text{Reconciliação}
+
\text{Evidência}
=
\text{Coerência evolutiva}
}
\]

Essa é a hipótese que SYNTH-0 deverá tentar refutar.

---

# 42. Próximo artefato sugerido

O documento seguinte deve ser:

```text
SYNTH-BOOTSTRAP-001
```

e deverá transformar esta direção em um patch executável contendo:

- árvore exata;
- contratos TOML;
- JSON Schemas;
- interfaces HTTP;
- estados;
- CMake;
- dependências;
- testes de aceitação;
- UX contract;
- comandos;
- critérios de commit.

Esse artefato deve ser suficientemente preciso para que um agente de implementação possa criar o bootstrap em **um único patch**, sem tomar decisões arquiteturais novas.
