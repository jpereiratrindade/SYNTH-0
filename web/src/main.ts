import './style.css';

type SystemWitness = {
  id: string;
  authority: string;
  reachable: boolean;
  witness: Record<string, unknown>;
};

type RelationWitness = {
  id: string;
  reachable: boolean;
  contract_ok: boolean;
  witness: Record<string, unknown>;
};

type Ecosystem = {
  evidence: { observed_at: string; systems: SystemWitness[]; relations: RelationWitness[] };
  plan: { action: string; reason: string; targets: string[] };
};

const position: Record<string, [number, number]> = {
  pulso: [300, 80],
  trama: [490, 320],
  lente: [110, 320]
};

class SynthTopology extends HTMLElement {
  private data?: Ecosystem;
  private source?: EventSource;
  private selected = 'ecosystem';

  connectedCallback() {
    this.render();
    this.connect();
  }

  disconnectedCallback() { this.source?.close(); }

  private connect() {
    this.source = new EventSource('/events');
    this.source.addEventListener('ecosystem', (event) => {
      this.data = JSON.parse((event as MessageEvent).data) as Ecosystem;
      this.render();
    });
    this.source.onerror = () => { void this.refresh(); };
    void this.refresh();
  }

  private async refresh() {
    try {
      const response = await fetch('/api/v1/ecosystem');
      if (response.ok) this.data = await response.json() as Ecosystem;
      this.render();
    } catch { /* factual absence remains visible */ }
  }

  private async disturb(id: string) {
    await fetch(`/api/v1/disturb/kill/${id}`, { method: 'POST' });
    await this.refresh();
  }

  private node(system: SystemWitness) {
    const [x, y] = position[system.id] ?? [300, 200];
    const state = system.reachable ? 'ready' : 'degraded';
    return `<button class="node ${state}" data-select="${system.id}" style="--x:${x}px;--y:${y}px" aria-label="${system.id}: ${state}">
      <span class="authority">${system.authority}</span>
      <strong>${system.id.toUpperCase()}</strong>
      <span class="state">${state}</span>
    </button>`;
  }

  private relation(id: string, from: string, to: string, label: string) {
    const r = this.data?.evidence.relations.find(x => x.id === id);
    const a = position[from]; const b = position[to];
    if (!a || !b) return '';
    const degraded = !r?.reachable || !r?.contract_ok;
    const x1 = a[0] + 65, y1 = a[1] + 38, x2 = b[0] + 65, y2 = b[1] + 38;
    return `<g class="edge ${degraded ? 'degraded' : 'ready'}" data-select="${id}" tabindex="0" role="button" aria-label="${id}">
      <line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" />
      <text x="${(x1+x2)/2}" y="${(y1+y2)/2 - 8}">${label}</text>
    </g>`;
  }

  private detail() {
    if (!this.data) return '<p>Sem testemunha factual ainda.</p>';
    const system = this.data.evidence.systems.find(x => x.id === this.selected);
    const relation = this.data.evidence.relations.find(x => x.id === this.selected);
    const value = system ?? relation ?? { plan: this.data.plan, evidence_time: this.data.evidence.observed_at };
    return `<pre>${escapeHtml(JSON.stringify(value, null, 2))}</pre>`;
  }

  private bind() {
    this.querySelectorAll<HTMLElement>('[data-select]').forEach(el => {
      el.onclick = () => { this.selected = el.dataset.select ?? 'ecosystem'; this.render(); };
      el.onkeydown = (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); el.click(); } };
    });
    this.querySelectorAll<HTMLButtonElement>('[data-kill]').forEach(el => {
      el.onclick = () => void this.disturb(el.dataset.kill ?? '');
    });
  }

  private render() {
    const systems = this.data?.evidence.systems ?? [];
    const action = this.data?.plan.action ?? 'UNKNOWN';
    const reason = this.data?.plan.reason ?? 'aguardando observação factual';
    this.innerHTML = `
      <main>
        <header>
          <div><span class="eyebrow">SYNTH-0 / EXPERIMENTO</span><h1>Living Topology</h1></div>
          <div class="plan"><span>PLANO ATUAL</span><strong>${action}</strong><small>${reason}</small></div>
        </header>
        <section class="stage" aria-label="Topologia factual do ecossistema">
          <svg viewBox="0 0 620 450" aria-hidden="false">
            ${this.relation('pulso-to-trama','pulso','trama','fatos')}
            ${this.relation('trama-to-lente','trama','lente','padrões')}
            ${this.relation('lente-to-pulso','lente','pulso','intenção')}
          </svg>
          ${systems.map(s => this.node(s)).join('')}
          <div class="center-mark"><span>DECLARED</span><span>OBSERVED</span><span>EVIDENCE</span></div>
        </section>
        <aside class="evidence">
          <div class="aside-title"><span>Evidence</span><button data-select="ecosystem">ecossistema</button></div>
          ${this.detail()}
        </aside>
        <section class="lab">
          <div><span class="eyebrow">DISTURBANCE LAB</span><h2>Perturbe. Observe. Reconcilie.</h2></div>
          <div class="controls">
            <button data-kill="pulso">Kill Pulso</button>
            <button data-kill="trama">Kill Trama</button>
            <button data-kill="lente">Kill Lente</button>
          </div>
        </section>
      </main>`;
    this.bind();
  }
}

function escapeHtml(value: string) {
  return value.replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;');
}

customElements.define('synth-topology', SynthTopology);
