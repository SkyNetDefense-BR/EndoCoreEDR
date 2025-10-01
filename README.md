# Documentação Técnica — EndoCoreEDR

## 1. Como funciona?
EDR composto por três camadas:
- **Driver (kernel):** canal seguro, proteção anti‑tamper e aplicação de políticas críticas.
- **Hooking DLL:** injetada em processos para observar/interceptar pontos relevantes (callbacks de APIs, carregamento de módulos) e oferecer um ponto de controle local.
- **Agent (userland):** toma decisões com base em telemetria e instrui driver/hook para remediar (isolar, desativar, quarentenar, notificar).

Objetivo: detectar carregamentos/ações suspeitas (especialmente DLLs) e executar remediação segura, preservando estabilidade do sistema e integridade do agente.


## 2. Componentes principais

### 2.1 Driver (Kernel)
- **Funções (alto nível):**
  - Monitora eventos do sistema (ex.: carregamento de imagens, criação de processos, operações em arquivos).
  - Provê canal seguro de comunicação com o Agent (objeto de dispositivo, IOCTLs autenticados).
  - Aplica políticas e proteção anti‑tamper ao Agent.
  - Registra eventos críticos.
- **Propriedades desejadas:** validação de entrada, princípio do menor privilégio, logging auditável.

### 2.2 Hooking DLL
- **Funções (alto nível):**
  - Intercepta APIs de interesse (ex.: `LoadLibrary` ou outras relacionadas a carregamento/execução).
  - Coleta contexto e dispara eventos seguros ao Agent/Driver.
  - Oferece mecanismos controlados para que o Agent neutralize ou marque módulos suspeitos.
- **Propriedades desejadas:** impacto mínimo na estabilidade do processo; interfaces limitadas e seguras.

### 2.3 Agent (Userland)
- **Funções (alto nível):**
  - Recebe telemetria, avalia com políticas/heurísticas e decide ações.
  - Orquestra remediações seguras (quarentena, isolar, bloquear I/O, instruir hook para neutralizar).
  - Mantém logs, estado e integração com consoles de gestão.
- **Propriedades desejadas:** autenticação mútua com driver, regras configuráveis, princípio do menor privilégio.


## 3. Arquitetura e fluxo de dados
    +----------------+         secure IPC         +-------------+
    |   User Space   | <------------------------> |   Driver    |
    |  (Agent + UI)  |                           |  (Kernel)   |
    +----------------+                           +-------------+
          ^  ^                                        ^
          |  | hooks/IPC                              |
          |  |                                         |
    +-----+--+-----+     injected hooking DLLs        |
    | Processes   | <-------------------------------+
    | (apps, svc) |
    +-------------+

    Passos típicos:
1. Processo carrega uma DLL.
2. Hooking DLL (ou driver via notificação de imagem) detecta e coleta metadados.
3. Evento enviado ao Agent por canal seguro.
4. Agent avalia e instrui driver/hook para remediação se necessário.
5. Logs e auditoria são gerados.


## 4. Callbacks — como funcionam e como a EDR os usa

### O que é um *callback* aqui
Callback = rotina que notifica o EDR quando um evento relevante acontece (carregamento de DLL, criação de processo, operação em arquivo, chamada API sensível). Callbacks podem ocorrer em níveis diferentes: kernel, userland (hooking DLL) e agent.

### Tipos de callbacks
1. **Kernel-level callbacks**
   - Notificações nativas do SO (ex.: criação/terminação de processo, carregamento de imagem).
   - O driver registra handlers e recebe eventos confiáveis e de baixo nível.
   - Úteis para garantir deteção mesmo quando userland é manipulado.

2. **Userland hooks / callbacks na Hooking DLL**
   - A Hooking DLL intercepta APIs dentro do processo (p.ex. `LoadLibrary`) e executa uma rotina callback local.
   - Essa rotina coleta contexto (process id, stack, parâmetros), envia evento ao Agent/Driver e pode aplicar mitigação local imediata.
   - Deve ser implementada para minimizar impacto no processo.

3. **Callbacks do Agent**
   - Depois de receber um evento, o Agent executa callbacks internos (ex.: análise, política, acionamento de playbooks).
   - Pode haver callbacks de follow-up (p.ex. quando quarentena completa ou quando o estado do arquivo muda).

### Fluxo de uso de callbacks (simplificado)
1. Evento: processo carrega DLL → hook/kernel dispara callback que coleta `path`, `hash`, `pid`, `timestamp`.
2. Envio: evento autenticado ao Agent (via driver/IPC seguro).
3. Triagem: Agent aplica regras e decide ação.
4. Remediação: Agent aciona callbacks de remediação para Hooking DLL e/ou Driver.
5. Auditoria: todas callbacks e ações são logadas com `event_id`/`trace_id`.

### Princípios ao trabalhar com callbacks
- **Validação e autenticação:** validar mensagens de userland no kernel antes de agir.
- **Callbacks curtos:** não executar trabalho pesado no caminho crítico — delegar ao Agent.
- **Fail-safe:** no erro, preferir modo observador em vez de causar corrupção.
- **Rate limiting e deduplicação:** evitar DoS por flood de eventos.
- **Rollback seguro:** permitir reversão quando possível.
- **Separação de privilégios:** kernel aplica políticas; userland coleta contexto.
- **Auditoria:** correlacionar eventos e ações.

### Exemplos de ações atreladas a callbacks (alto nível)
- Callback de carregamento de DLL → Agent avalia → instruir hook a bloquear `LoadLibrary` ou marcar para quarentena.
- Callback de criação de processo → coletar cadeia de processo, checar hashes, prevenir spawning se necessário.
- Callback de operação em arquivo executável → driver pode negar escrita/rename temporariamente.

> Observação: O documento evita detalhes de implementação de hooking para não facilitar técnicas de abuso.


## 5. Responsabilidades detalhadas

### Driver
- Autorizar mensagens do Agent; validar inputs; proteger Agent (evitar `TerminateProcess`, injeção).
- Gerenciar regras em kernel-space (whitelist/blacklist).
- Registrar tentativas de manipulação e eventos críticos.

### Hooking DLL
- Interceptar pontos-chave sem expor interfaces inseguras.
- Fornecer mecanismo seguro para que Agent aplique alterações na DLL alvo.
- Garantir rollback seguro em caso de falha.

### Agent
- Tomada de decisão centralizada, auditável.
- Políticas configuráveis (monitor vs bloqueio).
- Orquestra remediações e envio de telemetria.


## 6. Protocolos / IPC (alto nível)
- **Canal kernel ↔ user:** dispositivo/IOCTLs autenticados; checagem de assinatura do agente; mensagens assinadas.
- **Hooking DLL ↔ Agent:** comunicação mediada por driver ou canal local autenticado; mensagens curtas contendo `event_id`, `pid`, `module_path`, `module_hash`, `timestamp`.
- **Formato de mensagem sugerido:** JSON/CBOR/Protobuf (p.ex. campos mínimos: `event_id`, `process_id`, `module_path`, `module_hash`, `severity`, `timestamp`, `context`).



## 7. Cenários de operação — exemplos

### 7.1 Detecção de DLL suspeita carregada
- Hook detecta `LoadLibrary` → coleta metadata → envia evento ao Agent → Agent decide: observar / bloquear / neutralizar → se bloqueio, Driver impede execução adicional.

### 7.2 Neutralização em memória
- Agent pode instruir Hook a inativar pontos de export ou interceptar chamadas; ou instruir Driver para isolar/derrubar processo se for um risco alto.

### 7.3 Remoção do artefato no disco
- Agent agenda quarentena/removal seguro (p.ex., agendamento para próximo reboot) e gera auditoria.



---
## Instalação
Este documento descreve os passos necessários para configurar um ambiente de desenvolvimento para uma solução de EDR (Endpoint Detection and Response) baseada em **driver de kernel no Windows**, incluindo instalação do Windows Driver Kit (WDK) e configuração de um ambiente de depuração de kernel com VirtualBox.

## Links de referência usados
- Windows Driver Kit (WDK): https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
- Debugging / Kernel debug lab for VirtualBox: https://github.com/xalicex/kernel-debug-lab-for-virtual-box

## Requisitos
- Host: Windows 10/11 (preferencialmente Pro/Enterprise)
- VirtualBox (para VMs de teste)
- Visual Studio 2022 ou superior (Community é suficiente)
- Windows SDK correspondente à versão alvo
- WDK compatível com o SDK instalado
- Git (opcional)
- vcpkg
- WinDbg / WinDbg Preview (para depuração)

## 1. Instalação do ambiente de desenvolvimento

### 1.1 Visual Studio
Instalar Visual Studio 2022 com os workloads:
- Desktop development with C++
- Ferramentas para desenvolvimento de drivers (se disponível no instalador)

### 1.2 Windows Driver Kit (WDK)
Baixar e instalar o WDK da Microsoft:
- URL: https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

> Nota: Instale a versão do WDK compatível com o Windows SDK que você pretende usar.

### 1.3 Ferramentas adicionais
- WinDbg Preview (Microsoft Store) — para depuração de kernel e user-mode.
- OSR Driver Loader (opcional) — facilita instalação de drivers de teste.
- Git — para clonar repositórios de configuração de VM e scripts.

## 2. Estrutura básica de um driver EDR
Estrutura sugerida de diretórios e arquivos:

### Principais componentes
- `DriverEntry` / `DriverUnload`
- Rotinas de proteção e monitoramento (callbacks / MiniFilter / PsSetCreateProcessNotifyRoutineEx, etc.)
- Comunicação com modo usuário via DeviceIoControl / IOCTLs
- Mecanismos de logging seguro (buffers, limites)
- Mecanismo de configuração (parametros passados via user-mode)

## 3. Boas práticas de desenvolvimento
- Evitar operações longas em contexto de interrupção (IRQL alto).
- Validar todos os ponteiros e buffers vindos do modo usuário.
- Tratar corretamente cancelamento e sincronização (mutexes/fast mutex/spinlocks quando apropriado).
- Implementar timeout / limites para evitar deadlocks.
- Separar lógica crítica (kernel) de análise pesada (user-mode) quando possível.
- Registrar eventos e logs com cautela (não expor dados sensíveis).

## 4. Ambiente de teste e depuração com VirtualBox

### 4.1 Repositório de referência
- kernel-debug-lab-for-virtual-box: https://github.com/xalicex/kernel-debug-lab-for-virtual-box

### 4.2 Passos resumidos
1. Clone o repositório:
  git clone https://github.com/xalicex/kernel-debug-lab-for-virtual-box.git
2. Use os scripts presentes para criar uma VM Windows no VirtualBox.
3. Configure a VM para depuração serial:
- Dentro da VM (ou via configuração), habilitar depuração:
  ```
  bcdedit /debug on
  bcdedit /dbgsettings serial debugport:1 baudrate:115200
  ```
4. No host, abrir WinDbg Preview e conectar via porta COM (ex.: COM1, 115200).
- Em WinDbg: File → Kernel Debug → COM → configurar porta e baudrate.
5. Boot da VM; WinDbg deve conectar e permitir breakpoints de kernel, stacks, logs, etc.

### 4.3 Dicas
- Habilite testesigning para desenvolvimento local (não recomendado em produção):

## 5. Compilação e instalação do driver

### 5.1 Compilando
1. Abrir o projeto no Visual Studio (configurar plataforma x64).
2. Build → selecionar Release x64.
3. O `.sys` será gerado em `x64\Release\` (ou pasta equivalente).

### 5.2 Instalando
- Usando `sc`:

## 6. Assinatura e requisitos do Windows modernos
- Para produção em Windows 10/11 com Secure Boot, drivers devem ser assinados.
- Assinatura cross-signed / EV certificate / portal da Microsoft (dependendo do caso).
- Em ambientes de desenvolvimento, `testsigning` é uma alternativa temporária:


## 7. Segurança e privacidade
- Minimizar coleta de dados sensíveis no kernel.
- Usar criptografia / canários / hashes para comunicação entre componentes.
- Política de retenção e envio de logs: preferir triagem em user-mode antes de envio para servidores.
- Planejar atualizações de driver (mecanismo de atualização seguro).

## 8. Monitoramento típico de EDR em kernel
- Process creation/termination hooks
- File system monitoring (MiniFilter para interceptar operações de arquivo)
- Registry monitoring
- Network hooks (quando necessário, usando filtros de transporte/NDIS conforme escopo)
- Análise de comportamento em user-mode, com kernel fornecendo eventos confiáveis

## 9. Testes e validação
- Testes de unidade para componentes user-mode.
- Testes de integração com VM e WinDbg (breakpoints e análise de stack).
- Fuzzing de IOCTLs (testar validações de parâmetros).
- Verificação de desempenho: medir overhead de chamadas monitoradas.

## 10. Recursos adicionais
- Microsoft Docs - Drivers: https://learn.microsoft.com/en-us/windows-hardware/drivers/
- WinDbg Preview (Microsoft Store)
- Repositório de debug para VirtualBox: https://github.com/xalicex/kernel-debug-lab-for-virtual-box

###
## 📸 Funcionamento 

#### 1. Criação de processo detectada:
<img width="1164" height="659" alt="Image" src="https://github.com/user-attachments/assets/6fae3db9-e655-4481-83b7-84449d483ae5" />

#### 2. Logs do driver:
<img width="1173" height="656" alt="Image" src="https://github.com/user-attachments/assets/b900c0f7-1d0b-408f-a065-8e751229ba37" />

#### 3. WinDbg conectado e breakpoint ativado:
<img width="1111" height="744" alt="Image" src="https://github.com/user-attachments/assets/84ebce52-2b7d-4337-9223-ca1fca729093" />
