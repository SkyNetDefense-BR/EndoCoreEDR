# Documentação Técnica — Ambiente de Desenvolvimento de Driver de Kernel para EDR

## Objetivo
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

#### 2. Logs do driver:

#### 3. WinDbg conectado e breakpoint ativado:
