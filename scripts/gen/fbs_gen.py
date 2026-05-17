from pathlib import Path
import re

type_map = {
    "bool" : "bool",
    "int" : "int64",
    "float" : "float64",
    "string" : "string"
}

devtype_path = Path("config/DEVTYPES.LIST")
schema_path = Path("gen/schemas")
schema_path.mkdir(parents=True, exist_ok=True)

with open(devtype_path) as devtype_list, open(schema_path / "devtypes.fbs", "w") as fbs:
    while (line := devtype_list.readline()):
        line = line.strip()
        if line.startswith("DEVTYPE_BEGIN"):
            devtype = line.removeprefix("DEVTYPE_BEGIN(").split(",")[0].strip()
            fbs.write(f"table {devtype} {{\n")
        elif line.startswith("DEVTYPE_END"):
            fbs.write("}\n\n")
        elif line.startswith("ATTRIBUTE"):
            parsed = line.removeprefix("ATTRIBUTE(").removesuffix(")").split(",")
            attribute = re.sub(r"(?=[A-Z])", "_", parsed[0].strip()).lower() # convert to snake_case
            tp = ",".join(parsed[1:]).strip()
            tp = type_map[tp]
            fbs.write(f"    {attribute}:{tp};\n")
