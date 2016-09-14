lsof -i tcp:7055 | awk 'NR!=1 {print $2}' | xargs kill -kill
