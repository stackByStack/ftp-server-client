'use client'
import '../globals.css'
import * as React from "react";
import { useMemo, useRef } from 'react';
import SignInSide from "../components/login";
import fileItemInfo from "../utils/fileItemInfo";
import FileList from "../components/fileList";
import RouteBread from "../components/routeBread";
import { connect, login, set_pasv, download, upload, dir, rename, mkdir, rmdir, pwd, deleteFile } from "../api/api";
import { LinearProgress } from "@mui/material";
import { notification } from 'antd';
import { createTheme, ThemeProvider } from '@mui/material/styles';
import CircularProgress from '@mui/material/CircularProgress';
import CssBaseline from '@mui/material/CssBaseline';
import RefreshIcon from '@mui/icons-material/Refresh';
import Switch from '@mui/material/Switch';
// import ftpSession from "../utils/ftplib";
import OpenIconSpeedDial from '../components/speedDial';
import moment, { now } from 'moment';

const errorapi = (api, message) => {
    api.error({
        message: 'error',
        description: message,
        placement: 'bottomLeft',
    });
}

const successapi = (api, message) => {
    api.success({
        message: 'success',
        description: message,
        placement: 'bottomLeft',
    });
}

const warningapi = (api, message) => {
    api.warning({
        message: 'warning',
        description: message,
        placement: 'bottomLeft',
    });
}

const infoapi = (api, message) => {
    api.info({
        message: 'info',
        description: message,
        placement: 'bottomLeft',
    });
}

const defaultTheme = createTheme();
export default function ftpPage() {
    const [isLogin, setIsLogin] = React.useState(false);
    const [isloading, setIsLoading] = React.useState(false);
    const [username, setUsername] = React.useState('');
    const [ftpPath, setFtpPath] = React.useState('/');
    const [polygonPoints, setPolygonPoints] = React.useState('');
    const [pasv, setPasv] = React.useState(true);
    const sessionId = React.useRef('');
    const fileInputRef = useRef(null);
    const [fileItems, setFileItems] = React.useState([]);
    const [api, contextHolder] = notification.useNotification();
    //Create a ftpSession and only once

    const loginSuccessCallback = async () => {
        setIsLogin(true);
        setFileItems([]);
        setIsLoading(true);
        try {
            const response = await dir(ftpPath, sessionId.current);
            if (response.status === 200) {
                //data.result is a list of string, change them into fileItemInfo
                const newFileItems = response.data.result.map((line) => new fileItemInfo(line));
                setFileItems(newFileItems);
                successapi(api, 'change path success');
            }
            else {
                errorapi(api, 'change path failed');
            }
        }
        catch (err) {
            console.log(err);
            errorapi(api, 'change path failed');
            setIsLoading(false);
        }
        setIsLoading(false);
    }
    // const loginFailCallback = (err) => {
    //     setIsLogin(false);
    // }
    const closeCallback = () => {
        setIsLogin(false);
    }


    const setNewPath = async (newPath) => {
        setFtpPath(newPath);
        setFileItems([]);
        setIsLoading(true);
        try {
            const response = await dir(newPath, sessionId.current);
            if (response.status === 200) {
                //data.result is a list of string, change them into fileItemInfo
                const newFileItems = response.data.result.map((line) => new fileItemInfo(line));
                setFileItems(newFileItems);
                successapi(api, 'change path success');
            }
        }
        catch (error) {
            console.log(error);
            errorapi(api, 'change path failed');
            setIsLoading(false);
        }
        setIsLoading(false);
    }

    const loginCallback = async (loginInfo) => {
        try {
            setIsLoading(true);
            try {
                const response = await connect(loginInfo.host, loginInfo.port);
                console.log(response.data.sessionId);
                if (response.status === 200) {

                    sessionId.current = response.data.sessionId;
                    setUsername(loginInfo.username);
                    setIsLoading(true);
                    try {
                        const response1 = await login(loginInfo.username, loginInfo.password, sessionId.current);
                        setIsLoading(false);
                        if (response1.status === 200) {
                            successapi(api, 'login success');
                            await loginSuccessCallback();
                        }
                        else {
                            errorapi(api, 'login failed');
                        }
                    }
                    catch (err) {
                        console.log(err);
                        errorapi(api, 'login failed');
                        setIsLoading(false);
                        return;
                    }
                }
            }
            catch (err) {
                console.log(err);
                errorapi(api, 'connect failed');
                setIsLoading(false);
                return;
            }


        }
        catch (err) {
            console.log(err);
            setIsLogin(false);
        }
    };

    const mkdirCallback = async (newDirName) => {
        let path = ftpPath;
        if (ftpPath[ftpPath.length - 1] !== '/') {
            path = ftpPath + '/';
        }
        path += newDirName;
        let nowpath = ftpPath;
        try {
            setIsLoading(true);
            const response = await mkdir(path, sessionId.current);
            setIsLoading(false);
            if (response.status === 200) {
                successapi(api, 'create directory success');
                if (nowpath === ftpPath) {
                    setNewPath(ftpPath);
                }
            }
            else {
                errorapi(api, 'create directory failed');
            }
        }
        catch (err) {
            console.log(err);
            errorapi(api, 'create directory failed');
        }

    }

    const handleFileChange = async (e) => {
        const files = e.target.files;
        // const nowpath = ftpPath;
        let haveOneSuccess = false;
        if (files.length !== 0) {
            for (let i = 0; i < files.length; i++) {
                let file = files[i];
                try {
                    //custom api with loading icon
                    const icon = (
                        <CircularProgress
                            color="inherit"
                            size={20}
                        />
                    );
                    const index = moment().format('YYYY-MM-DD_HH-mm-ss-SSS');
                    api.open({
                        message: 'uploading',
                        description: `uploading file ${file.name}...`,
                        icon: icon,
                        placement: 'bottomRight',
                        key: index,
                    });
                    console.log('nowpath ', ftpPath);
                    const response = await upload(file, ftpPath, sessionId.current);
                    api.destroy(index);
                    if (response.status === 200) {
                        successapi(api, `${file.name} upload success`);
                        haveOneSuccess = true;
                    }
                    else {
                        errorapi(api, `${file.name} upload failed`);
                    }
                }
                catch (err) {
                    console.log(err);
                    errorapi(api, `${file.name} upload failed`);
                }
            }
            // if (haveOneSuccess === true && nowpath === ftpPath) {
            //     console.log('ftp ', ftpPath);
            //     setNewPath(ftpPath);
            // }
        }
    }

    const handleUpload = () => {
        fileInputRef.current.click();
    }

    const handleDownloadClick = async (fileItem) => {
        if (fileItem.type === 'directory') {
            // setup new ftp path
            let path = ftpPath;
            if (ftpPath[ftpPath.length - 1] !== '/') {
                path = ftpPath + '/';
            }
            path += fileItem.name;
            await setNewPath(path);
            console.log(ftpPath);
        }
        else {
            let path = ftpPath;
            if (ftpPath[ftpPath.length - 1] !== '/') {
                path = ftpPath + '/';
            }
            path += fileItem.name;
            try {
                const icon = (
                    <CircularProgress
                        color="inherit"
                        size={20}
                    />
                );
                const index = moment().format('YYYY-MM-DD_HH-mm-ss-SSS');
                api.open({
                    message: 'downloading',
                    description: `downloading file ${fileItem.name}...`,
                    icon: icon,
                    placement: 'bottomRight',
                    key: index,
                });
                const response = await download(path, sessionId.current);
                api.destroy(index);
                if (response.status === 200) {
                    successapi(api, `${fileItem.name} download success`);
                }
                else {
                    errorapi(api, `${fileItem.name} download failed`);
                }
            }
            catch (err) {
                console.log(err);
                errorapi(api, `${fileItem.name} download failed`);
            }
        }
    }

    const handleDeleteClick = async (fileItem) => {
        if (fileItem.type === 'directory') {
            // setup new ftp path
            let path = ftpPath;
            if (ftpPath[ftpPath.length - 1] !== '/') {
                path = ftpPath + '/';
            }
            path += fileItem.name;
            try {
                setIsLoading(true);
                const response = await rmdir(path, sessionId.current);
                setIsLoading(false);
                if (response.status === 200) {
                    successapi(api, 'delete directory success');
                    await loginSuccessCallback();
                }
                else {
                    errorapi(api, 'delete directory failed');
                }
            }
            catch (err) {
                console.log(err);
                errorapi(api, 'delete directory failed');
            }
        }
        else {
            let path = ftpPath;
            if (ftpPath[ftpPath.length - 1] !== '/') {
                path = ftpPath + '/';
            }
            path += fileItem.name;
            try {
                setIsLoading(true);
                const response = await deleteFile(path, sessionId.current);
                setIsLoading(false);
                if (response.status === 200) {
                    successapi(api, 'delete file success');
                    await loginSuccessCallback();
                }
                else {
                    errorapi(api, 'delete file failed');
                }
            }
            catch (err) {
                console.log(err);
                errorapi(api, 'delete file failed');
            }
        }
    }

    const handleRenameCallback = async (oldName, newName) => {
        let path = ftpPath;
        if (ftpPath[ftpPath.length - 1] !== '/') {
            path = ftpPath + '/';
        }
        try {
            setIsLoading(true);
            const response = await rename(path + oldName, path + newName, sessionId.current);
            setIsLoading(false);
            if (response.status === 200) {
                successapi(api, 'rename success');
                await setNewPath(ftpPath);
            }
            else {
                errorapi(api, 'rename failed');
            }
        }
        catch (err) {
            console.log(err);
            errorapi(api, 'rename failed');
        }
    }
    const handlePasvChange = async (event) => {
        if (!pasv) {
            try {
                setIsLoading(true);
                const response = await set_pasv('true', sessionId.current);
                setIsLoading(false);
                if (response.status === 200) {
                    successapi(api, 'set PASV success');
                }
                else {
                    errorapi(api, 'set PASV failed');
                }
            }
            catch (err) {
                console.log(err);
                errorapi(api, 'set PASV failed');
            }
        }
        else {
            try {
                setIsLoading(true);
                const response = await set_pasv('false', sessionId.current);
                setIsLoading(false);
                if (response.status === 200) {
                    successapi(api, 'set PORT success');
                }
                else {
                    errorapi(api, 'set PORT failed');
                }
            }
            catch (err) {
                console.log(err);
                errorapi(api, 'set PORT failed');
            }
        }
        setPasv(!pasv);

    };


    const generateRandomPolygon = () => {
        const points = [];
        const numPoints = 16; // Number of points in the polygon

        for (let i = 0; i < numPoints; i++) {
            const x = Math.random() * 100; // Random x-coordinate between 0 and 100
            const y = Math.random() * 100; // Random y-coordinate between 0 and 100
            points.push(`${x}% ${y}%`);
        }

        setPolygonPoints(points.join(', '));
    };
    React.useEffect(() => {
        generateRandomPolygon(); // Generate initial polygon

        const interval = setInterval(() => {
            generateRandomPolygon(); // Generate new polygon at the interval
        }, 1000); // Interval in milliseconds (e.g., 5000ms = 5 seconds)

        return () => {
            clearInterval(interval); // Clean up the interval when the component unmounts
        };
    }, []);

    return (
        <main className="min-h-screen">
            <div
                className="absolute inset-x-0 -top-20 -z-10 transform-gpu overflow-hidden blur-3xl sm:-top-80"
                aria-hidden="true"
            >
                <div
                    className="relative left-[calc(80%-11rem)] aspect-[1155/678] w-[36.125rem] -translate-x-1/2 rotate-[40deg] bg-gradient-to-tr from-[#ff80b5] to-[#9089fc] opacity-30 sm:left-[calc(50%-30rem)] sm:w-[72.1875rem]"
                    style={{
                        clipPath: `polygon(${polygonPoints})`,
                        transition: 'clip-path 1s ease-in-out', // Transition effect
                    }}
                />
            </div>
            {contextHolder}
            {isloading ? <LinearProgress /> : null}
            {isLogin ? (
                <>
                    <ThemeProvider theme={defaultTheme}>
                        <CssBaseline />
                        <div className="py-3 px-3">
                            <div className="hidden sm:mb-8 sm:flex">
                                <div className="relative rounded-full px-3 py-1 text-sm leading-6 text-gray-600 ring-1 ring-gray-900/10 hover:ring-gray-900/20">
                                    <RouteBread username={username} ftpPath={ftpPath} setNewPath={setNewPath} />

                                </div>
                                <div className="relative rounded-full px-3 py-1 text-sm leading-6 text-gray-600 ring-1 ring-gray-900/10 hover:ring-gray-900/20">
                                    {pasv ? 'PASV' : null}
                                    <Switch
                                        checked={pasv}
                                        onChange={handlePasvChange}
                                        inputProps={{ 'aria-label': 'controlled' }}
                                    />
                                    {pasv ? null : 'PORT'}
                                </div>
                                <RefreshIcon className={isloading ? 'rotate' : ''} onClick={() => setNewPath(ftpPath)} style={{ paddingLeft: 10, cursor: 'pointer', transition: 'all 0.1s ease-in' }} />
                            </div>

                        </div>
                        <FileList fileItems={fileItems} handleDownloadClick={handleDownloadClick} handleDeleteClick={handleDeleteClick} handleRenameCallback={handleRenameCallback} />
                        <input
                            type="file"
                            ref={fileInputRef}
                            style={{ display: 'none' }}
                            onChange={handleFileChange}
                            multiple
                        />
                        <OpenIconSpeedDial createNewFolderCallback={mkdirCallback} uploadCallback={handleUpload} />
                    </ThemeProvider>


                </>
            ) : (
                <div className="mx-auto px-3 py-3">
                    <div className="relative rounded-md px-3 py-3 text-sm leading-3 text-gray-600 ring-1 ring-gray-900/10 hover:ring-gray-900/20">
                        <SignInSide callback={loginCallback} />
                    </div>
                </div>

            )}

        </main>
    );
}