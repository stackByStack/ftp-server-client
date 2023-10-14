'use client'
import * as React from 'react';
import Typography from '@mui/material/Typography';
import Breadcrumbs from '@mui/material/Breadcrumbs';
import Link from '@mui/material/Link';

export default function RouteBread({ username, ftpPath, setNewPath }) {
    const paddingName = username + ftpPath;
    const pathList = paddingName.split('/');
    let link = '/';
    
    const getLink = (path, index) => {
        link = '/';
        for(let i = 1; i <= index; i++) {
            link += pathList[i] + '/';
        }
        return link;
    }

    return (
        <Breadcrumbs aria-label="breadcrumb">
            {pathList.map((path, index) => {
                if (path === '') {
                    return (
                        <Typography key={index} color="text.primary">
                            
                        </Typography>
                    );
                }
                else {
                    return (
                        <Link
                            key={index}
                            underline="hover"
                            color="inherit"
                            href='#'
                            onClick={() => { setNewPath(getLink(path, index)) }}
                        >
                            {path}
                        </Link>
                    );
                }
            })}
        </Breadcrumbs>
    );

}