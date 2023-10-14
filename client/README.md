# 1. How to start 
## 1.1 setup
- backend:

```bash
cd src/backend
pip install -r requirements.txt
```
- frontend:

```bash
cd src/young-ftp
npm install
```
## 1.2 
You can separatively deploy them. 

- backend: (add -debug if you like)
```bash
python -m flask --app server run
```

- frontend
dev:

```bash
npm run dev
```

deploy:

```bash
npm run build
```

