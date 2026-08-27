"""Script PlatformIO : régénère l'en-tête de configuration avant chaque build.

Garantit qu'aucun binaire ne part au flash avec un `generated_profile.h`
périmé : le contraire signifierait flasher des timings qui ne sont pas ceux du
profil relevé.

Le profil utilisé vient de la variable d'environnement AGV_PROFILE, sinon
`profiles/default.yaml`.
"""

import os
import subprocess
import sys

Import("env")  # noqa: F821  (injecté par PlatformIO)

project_dir = env.subst("$PROJECT_DIR")  # noqa: F821
profile = os.environ.get("AGV_PROFILE", os.path.join(project_dir, "profiles", "default.yaml"))
output = os.path.join(project_dir, "firmware", "common", "config", "generated_profile.h")

result = subprocess.run(
    [sys.executable, os.path.join(project_dir, "tools", "genconfig.py"), profile, output],
    check=False,
)
if result.returncode != 0:
    raise SystemExit(f"genconfig a échoué sur {profile} : build interrompu")
