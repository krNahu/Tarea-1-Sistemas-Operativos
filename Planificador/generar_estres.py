import random

NUM_TAREAS = 10000
ARCHIVO_SALIDA = "plan_estres.txt"

ACCIONES = [
    "prender_carbon",
    "comprar_carne",
    "comprar_pan",
    "asar_longaniza",
    "armar_choripan",
    "servir_mesa"
]

with open(ARCHIVO_SALIDA, "w", encoding="utf-8") as f:
    for i in range(1, NUM_TAREAS + 1):
        accion_base = random.choice(ACCIONES)

        if random.random() < 0.01 and i > 100:  
            nombre = f"falla_{accion_base}_{i}"
        else:
            nombre = f"{accion_base}_{i}"


        if random.random() < 0.5:
            tiempo = str(random.randint(5, 50))
        else:
            tiempo = ""

        deps = []
        if i == 1:
            pass
        elif i <= 10:
            deps.append(str(i - 1))
        else:
            num_deps = random.randint(1, 3)
            candidatas = random.sample(range(1, i), num_deps)
            deps = [str(d) for d in candidatas]

        dependencias_str = ", ".join(deps)

        linea = f"{i} : {nombre} : {tiempo} : {dependencias_str}\n"
        f.write(linea)

print(f"Archivo generado exitosamente")
