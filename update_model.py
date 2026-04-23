import json

with open('/home/lavenda/Projects/AdminGas/backend/api_server/models/model.json', 'r') as f:
    text = f.read()

import re
# It has comments, so standard json parser might fail. We will use Regex.
text = re.sub(r'"dbname":\s*".*?"', '"dbname": "admingas"', text)
text = re.sub(r'"user":\s*".*?"', '"user": "postgres"', text)
text = re.sub(r'"password":\s*".*?"', '"password": "admingas_password"', text)

with open('/home/lavenda/Projects/AdminGas/backend/api_server/models/model.json', 'w') as f:
    f.write(text)

