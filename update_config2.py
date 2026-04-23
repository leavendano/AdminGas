import re

with open('/home/lavenda/Projects/AdminGas/backend/api_server/config.json', 'r') as f:
    text = f.read()

text = re.sub(r'"user":\s*"postgres",\s*"passwd":\s*""', '"user": "postgres",\n            "passwd": "admingas_password"', text)

with open('/home/lavenda/Projects/AdminGas/backend/api_server/config.json', 'w') as f:
    f.write(text)

