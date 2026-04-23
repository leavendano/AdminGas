import re

with open('/home/lavenda/Projects/AdminGas/backend/api_server/config.json', 'r') as f:
    text = f.read()

replacement = """
    "listeners": [
        {
            "address": "0.0.0.0",
            "port": 8080,
            "https": false
        }
    ],
    "db_clients": [
        {
            "name": "default",
            "rdbms": "postgresql",
            "host": "localhost",
            "port": 5432,
            "dbname": "admingas",
            "user": "postgres",
            "passwd": "",
            "number_of_connections": 5
        }
    ],
"""

text = re.sub(r'/\*\s*//ssl:.*?],?\*/', replacement, text, flags=re.DOTALL)

with open('/home/lavenda/Projects/AdminGas/backend/api_server/config.json', 'w') as f:
    f.write(text)
