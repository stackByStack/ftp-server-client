import axios from 'axios'
const baseUrl = 'http://8.142.111.243/api'

/**
 * Connect to a server using the specified host and port.
 *
 * @param {string} host - The hostname or IP address of the server to connect to.
 * @param {number} port - The port number to use for the connection.
 * @returns {Promise} A promise that resolves to the response from the server after connecting.
 */

export const connect = async (host, port) => {
    const response = await axios.post(`${baseUrl}/connect`, { host: host, port: port })
    //return json
    return response
}

/**
 * Log in to a server with the provided username, password, and session ID.
 *
 * @param {string} username - The username for authentication.
 * @param {string} password - The password for authentication.
 * @param {string} sessionId - The session ID for the login.
 * @returns {Promise} A promise that resolves to the response from the server after logging in.
 */
export const login = async (username, password, sessionId) => {
    const response = await axios.post(`${baseUrl}/login`, { username: username, password: password, sessionId: sessionId })
    return response
}

/**
 * Set the PASV (Passive) mode with the specified configuration and session ID.
 *
 * @param {string} pasv - A boolean indicating whether to enable or disable PASV mode.
 * @param {string} sessionId - The session ID associated with the PASV mode configuration.
 * @returns {Promise} A promise that resolves to the response from the server after setting PASV mode.
 */
export const set_pasv = async (pasv, sessionId) => {
    const response = await axios.post(`${baseUrl}/setpassive`, { pasv: pasv, sessionId: sessionId })
    return response
}

/**
 * Downloads a file from the server.
 * @async
 * @param {string} path - The path of the file to download.
 * @param {string} sessionId - The session ID of the user.
 * @param {object} api - The API object to handle errors.
 * @returns {Promise<object>} - The response object from the server.
 */
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

/**
 * Upload a file to the server at the specified path using a session ID.
 *
 * @param {File} file - The file to upload.
 * @param {string} path - The destination path on the server.
 * @param {string} sessionId - The session ID for the upload.
 * @returns {Promise} A promise that resolves to the response from the server after uploading the file.
 */
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

/**
 * Retrieve a directory listing for the specified path using a session ID.
 *
 * @param {string} path - The path for which to retrieve the directory listing.
 * @param {string} sessionId - The session ID for the directory listing request.
 * @returns {Promise} A promise that resolves to the response from the server containing the directory listing.
 */
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

/**
 * Delete a file at the specified path using a session ID.
 *
 * @param {string} path - The path of the file to be deleted.
 * @param {string} sessionId - The session ID for the delete operation.
 * @returns {Promise} A promise that resolves to the response from the server after deleting the file.
 */
export const deleteFile = async (path, sessionId) => {
    const response = await axios.post(`${baseUrl}/delete`, { path: path, sessionId: sessionId })
    return response
}
