import axios from 'axios'
const baseUrl = 'http://localhost:5000'

export const connect = async (host, port) => {
    const response = await axios.post(`${baseUrl}/connect`, { host: host, port: port })
    //return json
    return response
}

export const login = async (username, password, sessionId) => {
    const response = await axios.post(`${baseUrl}/login`, { username: username, password: password, sessionId: sessionId })
    return response
}

export const set_pasv = async (pasv, sessionId) => {
    const response = await axios.post(`${baseUrl}/set_pasv`, { pasv: pasv, sessionId: sessionId })
    return response
}

export const download = async (path, sessionId, api) => {
    try {
        const response = await axios.post(`${baseUrl}/download`, {
            path: path,
            sessionId: sessionId,
        }, {
            responseType: 'blob',
        });

        if (response.status === 200) {
            const blob = new Blob([response.data]);
            const filename = path.substring(path.lastIndexOf('/') + 1);

            // Create a temporary anchor element
            const link = document.createElement('a');
            link.href = window.URL.createObjectURL(blob);
            link.download = filename;

            // Trigger the download
            link.click();

            // Clean up the temporary anchor element
            link.remove();
        }

        return response;
    } catch (error) {
        console.error('Error downloading file:', error);
        api.error({
            message: 'Error',
            description: 'Error downloading file:' + error,
            placement: 'topLeft',
        })
    }
}

export const upload = async (file, path, sessionId) => {
    const formData = new FormData();
    formData.append('file', file);
    formData.append('path', path);
    formData.append('sessionId', sessionId);

    const response = await axios.post(`${baseUrl}/upload`, formData, {
        headers: {
            'Content-Type': 'multipart/form-data',
        },
    });

    return response;
}

export const dir = async (path, sessionId) => {
    const response = await axios.post(`${baseUrl}/dir`, { path: path, sessionId: sessionId })
    return response
}

export const rename = async (oldPath, newPath, sessionId) => {
    const response = await axios.post(`${baseUrl}/rename`, { oldPath: oldPath, newPath: newPath, sessionId: sessionId })
    return response
}

export const mkdir = async (path, sessionId) => {
    const response = await axios.post(`${baseUrl}/mkdir`, { path: path, sessionId: sessionId })
    return response
}

export const rmdir = async (path, sessionId) => {
    const response = await axios.post(`${baseUrl}/rmdir`, { path: path, sessionId: sessionId })
    return response
}

export const pwd = async (sessionId) => {
    const response = await axios.post(`${baseUrl}/pwd`, { sessionId: sessionId })
    return response
}

export const deleteFile = async (path, sessionId) => {
    const response = await axios.post(`${baseUrl}/delete`, { path: path, sessionId: sessionId })
    return response
}
