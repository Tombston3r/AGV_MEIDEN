'use strict';
// Banc EnOcean — aucune bibliothèque : la UniPi peut être hors réseau.

const $ = (id) => document.getElementById(id);
const etat = { boutons: [], capture: null, onglet: 'detecter', rebours: null };

// --- API --------------------------------------------------------------------
async function api(methode, url, corps) {
  const rep = await fetch(url, {
    method: methode,
    headers: corps ? { 'Content-Type': 'application/json' } : undefined,
    body: corps ? JSON.stringify(corps) : undefined,
  });
  const data = await rep.json().catch(() => ({}));
  if (!rep.ok) throw new Error(data.erreur || `HTTP ${rep.status}`);
  return data;
}

// --- Liste ------------------------------------------------------------------
function heure(epoch) {
  return new Date(epoch * 1000).toLocaleTimeString('fr-FR');
}

async function rafraichir() {
  const { boutons } = await api('GET', '/api/boutons');
  etat.boutons = boutons;
  $('compte').textContent = boutons.length;
  $('vide').hidden = boutons.length > 0;

  const liste = $('liste');
  liste.innerHTML = '';
  for (const b of boutons) {
    const li = document.createElement('li');
    li.dataset.code = b.code;
    li.innerHTML = `
      <span class="nom"></span>
      <span class="code"></span>
      <span class="vu" data-vu>jamais vu</span>
      <button class="danger" title="Retirer">Retirer</button>`;
    li.querySelector('.nom').textContent = b.nom;
    li.querySelector('.code').textContent = b.code;
    li.querySelector('button').onclick = async () => {
      if (!confirm(`Retirer « ${b.nom} » ?`)) return;
      await api('DELETE', `/api/boutons/${b.code}`);
      rafraichir();
    };
    liste.appendChild(li);
  }
}

// --- Fenêtres d'appui -------------------------------------------------------
function fenetre({ titre, code, heure: h, rssi_dbm, inconnu }) {
  const el = document.createElement('div');
  el.className = 'fenetre' + (inconnu ? ' inconnu' : '');
  el.innerHTML = `<div class="titre"></div>
                  <div class="ligne"></div>
                  <div class="ligne"></div>`;
  const [l1, l2] = el.querySelectorAll('.ligne');
  el.querySelector('.titre').textContent = titre;
  l1.textContent = code;
  l2.textContent = `${h}${rssi_dbm ? `  ·  ${rssi_dbm} dBm` : ''}`;
  $('fenetres').appendChild(el);

  // Cinq secondes : assez pour lire à un mètre, assez court pour ne pas
  // empiler les fenêtres quand on teste plusieurs boutons à la suite.
  setTimeout(() => {
    el.classList.add('sortie');
    setTimeout(() => el.remove(), 260);
  }, 5000);
}

function surligner(code) {
  const li = document.querySelector(`li[data-code="${code}"]`);
  if (!li) return;
  li.classList.add('actif');
  li.querySelector('[data-vu]').textContent = new Date().toLocaleTimeString('fr-FR');
  setTimeout(() => li.classList.remove('actif'), 2500);
}

// --- Flux d'événements ------------------------------------------------------
function ecouter() {
  const src = new EventSource('/api/evenements');

  src.onopen = () => {
    $('pastille').classList.add('ok');
    $('etat-texte').textContent = 'à l’écoute';
  };
  src.onerror = () => {
    $('pastille').classList.remove('ok');
    $('etat-texte').textContent = 'reconnexion…';   // EventSource réessaie seul
  };

  src.addEventListener('appui', (e) => {
    const d = JSON.parse(e.data);
    fenetre({ titre: d.nom, code: d.code, heure: heure(d.heure), rssi_dbm: d.rssi_dbm });
    surligner(d.code);
    majEtat();
  });

  src.addEventListener('inconnu', (e) => {
    const d = JSON.parse(e.data);
    fenetre({ titre: 'Bouton non enregistré', code: d.code,
              heure: heure(d.heure), rssi_dbm: d.rssi_dbm, inconnu: true });
    majEtat();
  });

  src.addEventListener('apprentissage', (e) => {
    const d = JSON.parse(e.data);
    etat.capture = d;
    $('attente').hidden = true;
    $('capture').hidden = false;
    $('capture-code').textContent = d.code;
    $('capture-rssi').textContent = `${d.rssi_dbm} dBm — ${heure(d.heure)}`;
    $('dernier').hidden = true;
    validerActif();
    if (!$('champ-nom').value) $('champ-nom').focus();
  });
}

async function majEtat() {
  try {
    const s = await api('GET', '/api/etat');
    $('etat-port').textContent = s.port;
    $('etat-appuis').textContent =
      `${s.appuis} appui${s.appuis > 1 ? 's' : ''}` +
      (s.inconnus ? ` · ${s.inconnus} non reconnu${s.inconnus > 1 ? 's' : ''}` : '');
  } catch { /* le bandeau d'état n'est pas critique */ }
}

// --- Fenêtre d'ajout --------------------------------------------------------
function validerActif() {
  const nom = $('champ-nom').value.trim();
  const id = etat.onglet === 'detecter'
    ? (etat.capture && etat.capture.code)
    : $('champ-id').value.trim();
  $('btn-valider').disabled = !(nom && id);
}

function ouvrirModale() {
  etat.capture = null;
  $('erreur').hidden = true;
  $('champ-id').value = '';
  $('champ-nom').value = '';
  $('capture').hidden = true;
  $('attente').hidden = false;
  $('modale').hidden = false;
  choisirOnglet('detecter');

  api('POST', '/api/apprentissage', { duree_s: 30 }).then((r) => {
    // Un bouton pressé juste AVANT l'ouverture est proposé : c'est le cas
    // courant au banc, on appuie puis on va l'enregistrer.
    if (r.dernier_appui && Date.now() / 1000 - r.dernier_appui.heure < 120) {
      const d = r.dernier_appui;
      $('dernier').hidden = false;
      $('dernier').innerHTML = `Dernier appui capté : <strong>${d.code}</strong>`;
      const b = document.createElement('button');
      b.className = 'secondaire';
      b.textContent = 'Utiliser';
      b.onclick = () => {
        etat.capture = d;
        $('attente').hidden = true;
        $('capture').hidden = false;
        $('capture-code').textContent = d.code;
        $('capture-rssi').textContent = `${d.rssi_dbm} dBm — ${heure(d.heure)}`;
        $('dernier').hidden = true;
        validerActif();
      };
      $('dernier').appendChild(b);
    }
    rebours(30);
  });
}

function rebours(s) {
  clearInterval(etat.rebours);
  $('compte-a-rebours').textContent = s;
  etat.rebours = setInterval(() => {
    s -= 1;
    $('compte-a-rebours').textContent = Math.max(0, s);
    if (s <= 0) clearInterval(etat.rebours);
  }, 1000);
}

function fermerModale() {
  clearInterval(etat.rebours);
  $('modale').hidden = true;
  api('POST', '/api/apprentissage/annuler').catch(() => {});
}

function choisirOnglet(nom) {
  etat.onglet = nom;
  document.querySelectorAll('.onglet').forEach((o) =>
    o.classList.toggle('actif', o.dataset.onglet === nom));
  $('pan-detecter').hidden = nom !== 'detecter';
  $('pan-saisir').hidden = nom !== 'saisir';
  validerActif();
}

async function enregistrer() {
  const id = etat.onglet === 'detecter'
    ? (etat.capture && etat.capture.code)
    : $('champ-id').value.trim();
  try {
    await api('POST', '/api/boutons', { id, nom: $('champ-nom').value });
    fermerModale();
    rafraichir();
  } catch (e) {
    $('erreur').textContent = e.message;
    $('erreur').hidden = false;
  }
}

// --- Démarrage --------------------------------------------------------------
$('btn-ajouter').onclick = ouvrirModale;
$('btn-annuler').onclick = fermerModale;
$('btn-valider').onclick = enregistrer;
$('champ-nom').oninput = validerActif;
$('champ-id').oninput = validerActif;
document.querySelectorAll('.onglet').forEach((o) => {
  o.onclick = () => choisirOnglet(o.dataset.onglet);
});
document.addEventListener('keydown', (e) => {
  if (e.key === 'Escape' && !$('modale').hidden) fermerModale();
});

rafraichir();
majEtat();
ecouter();
