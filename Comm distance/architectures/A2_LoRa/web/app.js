// Supervision temps réel du poste fixe.
//
// Transport : WebSocket (/ws), avec repli sur interrogation périodique de
// /api/state si la socket tombe. Le repli n'est PAS un mode nominal : il est
// signalé dans le bandeau de liaison, sinon une socket morte passerait pour un
// AGV silencieux.

const state = {
  socket: null,
  pollTimer: null,
  lastUpdate: 0,
};

const $ = (id) => document.getElementById(id);

function setLinkBadge(text, cls) {
  const badge = $('link-state');
  badge.textContent = text;
  badge.className = `badge ${cls}`;
}

function render(data) {
  state.lastUpdate = Date.now();

  $('station').textContent = data.valid ? data.station : '-';
  $('speed').textContent = data.valid ? data.speed : '-';
  $('motion').textContent = !data.valid
    ? '-'
    : data.moving
      ? 'en mouvement'
      : data.in_station
        ? 'en station'
        : 'arrêté';
  $('fault').textContent = !data.valid ? '-' : data.fault ? 'DÉFAUT' : 'aucun';
  $('fault').classList.toggle('alarm', Boolean(data.fault));

  // Fraîcheur : c'est l'indicateur le plus important de la page. Une valeur
  // ancienne signifie que l'affichage ne représente plus l'état réel de l'AGV.
  const age = data.telemetry_age_ms;
  $('freshness').textContent = data.valid ? `${(age / 1000).toFixed(1)} s` : 'aucune';
  $('freshness').classList.toggle('alarm', !data.valid || age > 10000);

  // Niveau Wi-Fi à hauteur d'antenne AGV. Sous −75 dBm, la liaison décroche en
  // mouvement : c'est le seuil du profil, pas une valeur figée dans la page.
  $('rssi').textContent = data.rssi_dbm ? `${data.rssi_dbm} dBm` : '-';
  $('rssi').classList.toggle('alarm', data.rssi_dbm !== 0 && data.rssi_dbm < -75);

  // Repli de sécurité : l'information la plus importante de cette architecture.
  // Tant qu'il est actif, l'AGV refuse toute nouvelle course.
  const safeStop = Boolean(data.safe_stop);
  $('safe-stop').textContent = !data.valid
    ? '-'
    : safeStop
      ? 'ACTIF : courses refusées'
      : 'inactif';
  $('safe-stop').classList.toggle('alarm', safeStop);

  $('c-sent').textContent = data.commands_sent;
  $('c-refused').textContent = data.commands_refused;
  $('c-acks').textContent = data.acks;
  $('c-nacks').textContent = data.nacks;
  $('c-enocean').textContent = data.enocean;
  $('c-unpaired').textContent = data.unpaired;
  $('c-duty').textContent = data.tx_refused_duty;
  $('c-rssi').textContent = `${data.rssi_dbm} dBm`;
  $('profile').textContent = data.profile;

  $('pair-feedback').textContent = data.pairing_active
    ? 'Mode appairage ouvert : appuyez sur le bouton à associer.'
    : '';
  $('feedback-warning').classList.toggle('hidden', Boolean(data.operator_feedback));
}

function connect() {
  const url = `ws://${window.location.host}/ws`;
  const socket = new WebSocket(url);
  state.socket = socket;

  socket.onopen = () => {
    setLinkBadge('temps réel', 'ok');
    if (state.pollTimer) {
      clearInterval(state.pollTimer);
      state.pollTimer = null;
    }
  };
  socket.onmessage = (event) => {
    try {
      render(JSON.parse(event.data));
    } catch (err) {
      console.error('trame de supervision illisible', err);
    }
  };
  socket.onclose = () => {
    setLinkBadge('mode dégradé (interrogation périodique)', 'warn');
    startPolling();
    setTimeout(connect, 5000);
  };
  socket.onerror = () => socket.close();
}

function startPolling() {
  if (state.pollTimer) return;
  state.pollTimer = setInterval(refreshOnce, 2000);
  refreshOnce();
}

async function refreshOnce() {
  try {
    const response = await fetch('/api/state', { cache: 'no-store' });
    render(await response.json());
  } catch (err) {
    setLinkBadge('poste injoignable', 'alarm');
  }
}

async function post(path, payload, feedbackId) {
  const feedback = $(feedbackId);
  try {
    const response = await fetch(path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    });
    const data = await response.json();
    // Un refus doit être visible : budget de rapport cyclique épuisé ou
    // transport occupé signifient que la commande n'est PAS partie.
    feedback.textContent = data.ok
      ? 'Commande transmise.'
      : 'Commande REFUSÉE par le poste (modem occupé ou indisponible).';
    feedback.className = `feedback ${data.ok ? 'ok' : 'alarm'}`;
  } catch (err) {
    feedback.textContent = 'Poste injoignable : commande non transmise.';
    feedback.className = 'feedback alarm';
  }
}

$('goto-form').addEventListener('submit', (event) => {
  event.preventDefault();
  post(
    '/api/goto',
    {
      station: Number($('goto-station').value),
      speed: Number($('goto-speed').value),
    },
    'command-feedback',
  );
});

$('stop-form').addEventListener('submit', (event) => {
  event.preventDefault();
  post('/api/stop', { purge: $('stop-purge').checked }, 'command-feedback');
});

$('pair-form').addEventListener('submit', (event) => {
  event.preventDefault();
  post(
    '/api/pair',
    {
      station: Number($('pair-station').value),
      speed: Number($('pair-speed').value),
    },
    'pair-feedback',
  );
});

// Vieillissement de l'affichage : sans nouvelle donnée, la page doit se
// déclarer périmée plutôt que d'afficher indéfiniment la dernière valeur.
setInterval(() => {
  if (state.lastUpdate && Date.now() - state.lastUpdate > 15000) {
    setLinkBadge('données périmées', 'alarm');
  }
}, 1000);

connect();
