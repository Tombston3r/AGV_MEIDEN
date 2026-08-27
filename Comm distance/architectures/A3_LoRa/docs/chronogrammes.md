# Chronogrammes du séquenceur trois phases

> Les durées notées `t_*` sont des **paramètres** (`profiles/*.yaml`), pas des
> constantes. Les valeurs indiquées ici sont celles du profil par défaut et
> restent PROVISOIRES tant que le relevé oscilloscope du §12.4/§12.5 n'est pas
> fait sur la V5.0.1.

## Phase ÉCRITURE

```
                 t_setup_us                     y22_write_ack_ms (timeout)
              |<----------->|              |<---------------------------->|
X94  ─────────┐
     type     └───────────────────────────────────────────────────────────
adresse ──────┐(10 bits posés simultanément)
X96..XA7      └───────────────────────────────────────────────────────────
vitesse ──────┐(4 bits)
X86..X91      └───────────────────────────────────────────────────────────
X92  ─────────┐                                                  ┌────────
     switch   └──────────────────────────────────────────────────┘ t_hold_us
X93  ──────────────────────┐                          ┌───────────────────
     strobe                └──────────────────────────┘
Y22  ─────────────────────────────────┐        ┌────────────────────────── 
     reading complete                 └────────┘
                                      ^
                                      | write_op_return = OK
```

- Étapes 1 à 4 posées **en une seule écriture bus** : `X94`, adresse, vitesse et
  `X92` partent ensemble. C'est ce qui donne son sens à `t_setup_us` : le
  compte-à-rebours démarre au dernier changement de donnée.
- `X93` ne monte qu'après `t_setup_us`. Un strobe trop précoce est ignoré par
  l'automate : l'écriture est perdue et rattrapée au réessai, au prix d'un
  aller-retour (`t_setup_trop_court_fait_perdre_la_premiere_ecriture`).
- Timeout `Y22` → `write_tries++`. Au-delà de `write_max_tries` :
  `write_op_return = TIMEOUT`, défaut `WRITE_TIMEOUT`, **sorties au repos**.
- Retombée : `X93` puis, après `t_hold_us`, `X92`. Jamais l'inverse.

## Phase DÉMARRAGE

```
                       y05_start_ack_ms (timeout)
              |<------------------------------------->|
X82  ─────────┐                              ┌──────────────────
     start    └──────────────────────────────┘ t_hold_us
Y05  ────────────────────────┐
     moving flag             └───────────────────────────────────
                             ^
                             | start_op_return = OK -> TRANSIT
```

Timeout `Y05` → nouveau front `X82`, jusqu'à `start_max_tries`. Ensuite :
`start_op_return = TIMEOUT`, défaut `START_TIMEOUT`, sorties au repos.

## Phase TRANSIT

```
Y23..Y34  ══X═══X═══X═══X═══X═══X═══>  position décodée en continu
Y05       ────────────────────────┐    moving flag retombe à l'arrivée
Y10       ─────────────────────────────┐  in station flag
Y03/Y21   ─────────────────────────────────  surveillés en permanence
          |<-------- y10_arrival_ms (timeout) -------->|
```

**Ordre de test important** : l'arrivée (`Y10 && !Y05`) est évaluée AVANT
`Y21`. À l'arrivée, l'automate lève aussi « pas de destination programmée »
puisqu'il vient de consommer la sienne ; tester `Y21` en premier
transformerait chaque arrivée en défaut.

## Phase ARRIVÉE

```
Y10  ─────┐
          └────────────────────────────────
X83  ──────────┐              ┌────────────
     stop      └──────────────┘ t_strobe_us
               ^              ^
               |              | stop_op_return = OK si Y05 est retombé
               | stop_tries++
```

Si `Y05` est encore actif à la retombée de `X83`, nouvelle impulsion, jusqu'à
`stop_max_tries`, puis défaut `STOP_TIMEOUT`.

Course terminée → dépilement de la course suivante (retour à l'étape 1), ou
`IDLE` si la file est vide, ou `SAFE_STOP` si un arrêt sûr a été demandé.

## Perte de liaison

Absence de trame valide pendant `link_watchdog_s` (30 s par défaut) :

1. `AgvApp` appelle `Sequencer::request_safe_stop(LinkLost)`.
2. La course **en cours va jusqu'au point d'arrêt suivant**, jamais de coupure
   en pleine allée, jamais d'état indéterminé.
3. Aucune course supplémentaire n'est lancée ; les sorties retombent au repos.
4. LED `FAULT` allumée jusqu'à acquittement opérateur (`clear_fault()`).

## Rappel de sûreté

Ces chronogrammes décrivent un **organe de commande**. L'arrêt d'urgence, les
bumpers et le scrutateur laser restent dans une chaîne indépendante conforme à
l'ISO 3691-4, hors de portée de ce firmware.
