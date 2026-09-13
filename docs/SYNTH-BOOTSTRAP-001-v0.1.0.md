# SYNTH-BOOTSTRAP-001 — Patch inaugural executável

```context-metadata+json
{
  "document": {
    "id": "SYNTH-BOOTSTRAP-001",
    "version": "0.1.0",
    "status": "implementation-candidate",
    "created_at": "2026-09-13",
    "ecosystem": "SYNTH-0",
    "parent": "SYNTH-ARCH-001/0.1.0"
  },
  "intent": "Materializar em um único root patch a menor realização executável capaz de testar três sistemas autônomos, relações declarativas, estado observado, reconciliação, pós-convergência, distúrbio, orçamento de recursos e uma projeção web factual.",
  "bootstrap_target": "/dev/cpp/SYNTH-0",
  "review": {
    "passes": 5,
    "lenses": [
      "fidelidade às invariantes arquiteturais",
      "fronteiras e independência dos três sistemas",
      "lifecycle, idempotência e pós-convergência",
      "CPU, memória, backpressure e falhas",
      "experiência web, bootstrap operacional e removibilidade futura"
    ]
  }
}
```

## 1. Decisão de bootstrap

O patch cria um **monorepo fisicamente único**, mas não um programa monolítico. Pulso,
Trama e Lente produzem executáveis diferentes, têm autoridades diferentes e não
incluem código privado uns dos outros. O único código C++ compartilhado entre
eles é o pacote de **contratos públicos**.

O controller é infraestrutura do ecossistema, não um quarto subsistema de
domínio.

## 2. Fonte de verdade e artefatos

```text
declarations/      estado desejado editável e versionado
.state/generated/  identidade da realização derivada
.evidence/         testemunha factual regenerável
```

O controller calcula um SHA-256 determinístico do conjunto declarado, ordenando
os caminhos relativos e normalizando CRLF para LF. O digest não transforma a
declaração em fato; apenas liga realização e declaração.

## 3. Lifecycle

```text
VALIDATE
  ↓
OBSERVE
  ↓
PLAN
  ↓
APPLY, somente quando necessário
  ↓
OBSERVE AGAIN
  ↓
PLAN AGAIN
  ↓
VERIFY
```

A segunda execução sem mudança deve ser `NO_OP` e terminar em `KEEP`.

## 4. Estados de reconciliação

```text
KEEP
REPAIR
RECONFIGURE
DEGRADED
BLOCKED
```

- `RECONFIGURE`: digest declarado diverge do digest realizado.
- `REPAIR`: ao menos um sistema obrigatório está factualmente inalcançável.
- `DEGRADED`: processos requeridos estão vivos, mas uma relação obrigatória não está satisfeita.
- `BLOCKED`: relação obrigatória apresenta incompatibilidade de contrato.
- `KEEP`: declaração, realização e observação estão coerentes.

## 5. Distúrbio

O bootstrap implementa `disturb kill <sistema>` usando `systemctl --user kill`.
Os serviços de domínio usam `Restart=no` de propósito: o distúrbio precisa ficar
observável até que o loop de reconciliação decida `REPAIR`. O systemd continua
sendo o supervisor factual dos processos; o controller não mantém um supervisor
paralelo.

## 6. Relações

```text
Pulso --facts--> Trama --patterns--> Lente --intent--> Pulso
```

A última relação é uma intenção explícita. Lente **não** escreve em Pulso. O
endpoint de intenção contém `applied=false`, preservando a fronteira de
autoridade desde o primeiro patch.

## 7. Orçamento computacional

Todas as declarações exigem `max_memory_mb` e `max_queue_items` finitos.
Pulso usa histórico limitado; Trama usa janela limitada; Lente não acumula
histórico. Nenhuma fila infinita é introduzida no bootstrap.

O controller observa `MainPID`, RSS e ticks de CPU via systemd + `/proc`, e compara RSS com o limite declarado. O patch não tenta impor `target_cpu_percent` via cgroup ainda: a variável permanece um alvo declarado para uma etapa posterior. Fingir enforcement seria pior do que declarar corretamente a lacuna.

## 8. Living Topology

O controller serve:

```text
GET  /api/v1/ecosystem
GET  /api/v1/evidence
GET  /events                  SSE
POST /api/v1/disturb/kill/:id
```

A UI usa TypeScript, Web Components, SVG, CSS e Vite. A topologia é alimentada
por evidence real. Arestas não são decorativas: seu estado deriva dos witnesses
de relação.

## 9. Dependências deliberadamente reutilizadas

- CMake + Ninja + CTest;
- systemd --user;
- HTTP/JSON;
- SSE;
- OpenSSL SHA-256;
- nlohmann/json;
- toml++;
- cpp-httplib;
- TypeScript + Vite + Web Components + SVG.

Nenhum parser TOML, framework HTTP, hash, supervisor ou framework web próprio é
introduzido.

## 10. Comandos canônicos

Após criar o repositório em `/dev/cpp/SYNTH-0`:

```bash
cd /dev/cpp/SYNTH-0
./bin/bootstrap-dev
./bin/synth run --target dev
./bin/synth run --target dev
systemctl --user start synth0-controller.service
```

Experimento de distúrbio:

```bash
./bin/synth disturb kill pulso
./bin/synth status
./bin/synth run --target dev
```

## 11. Critério de aceitação do root patch

O patch é aceito quando:

1. três executáveis de domínio compilam separadamente;
2. três autoridades são únicas;
3. três relações são declaradas e observadas;
4. primeiro `run` realiza ou repara o runtime;
5. pós-convergência produz `KEEP`;
6. segundo `run` é `NO_OP`;
7. `kill pulso` produz uma witness de indisponibilidade;
8. novo `run` produz `REPAIR` e retorna a `KEEP`;
9. a UI reproduz o estado factual e o plano atual;
10. nenhum subsistema escreve estado privado de outro.

## 12. Cinco revisões do patch

### R1 — Invariantes

Confirmado: declaração e observação continuam separadas; witness não é usada
como fonte declarativa; uma autoridade não é copiada por outro sistema.

### R2 — Independência

Confirmado: Pulso, Trama e Lente têm `CMakeLists.txt`, fontes e processos
próprios. O único include cruzando fronteiras é `synth/contracts`, que é a
fronteira pública intencional.

### R3 — Convergência

Confirmado: `run` não termina após `apply`; ele executa `OBSERVE_AGAIN`,
`PLAN_AGAIN` e `VERIFY`. Idempotência é propriedade testável, não promessa.

### R4 — Recursos e distúrbios

Confirmado: histórias/janelas são limitadas e o controller diferencia processo
vivo de relação satisfeita. O patch não inventa enforcement de CPU que ainda
não existe.

### R5 — Experiência e bootstrap

Confirmado: a UI consome evidence factual via SSE; o root patch é removível; os
três sistemas podem ser extraídos futuramente para repositórios próprios sem
mudar seu contrato HTTP público.

## 13. Decisões conscientemente deixadas para depois

- aplicação/adjudicação automática da intenção de Lente sobre Pulso;
- enforcement por cgroups dos orçamentos de CPU/memória;
- persistência histórica de evidence;
- semver range completo para contratos;
- autenticação/autorização;
- TLS;
- extração dos sistemas para repositórios separados.

Essas ausências são **incompletude declarada**, não dívida escondida.
