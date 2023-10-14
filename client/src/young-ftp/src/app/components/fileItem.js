'use client'
import * as React from 'react';
import Accordion from '@mui/material/Accordion';
import AccordionDetails from '@mui/material/AccordionDetails';
import AccordionSummary from '@mui/material/AccordionSummary';
import Typography from '@mui/material/Typography';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import FileCopyIcon from '@mui/icons-material/FileCopy';
import FolderIcon from '@mui/icons-material/Folder';
import ImageIcon from '@mui/icons-material/Image';
import MusicNoteIcon from '@mui/icons-material/MusicNote';
import DescriptionIcon from '@mui/icons-material/Description';
import InsertDriveFileIcon from '@mui/icons-material/InsertDriveFile';
import CodeIcon from '@mui/icons-material/Code';
import CloudDownloadOutlinedIcon from '@mui/icons-material/CloudDownloadOutlined';
import DeleteOutlineRoundedIcon from '@mui/icons-material/DeleteOutlineRounded';
import DriveFileRenameOutlineIcon from '@mui/icons-material/DriveFileRenameOutline';
import InputDialog from './inputDialog';
import './fileItem.css'



export default function FileItem({ fileItemInfo, handleDownloadClick, handleDeleteClick, handleRenameCallback }) {
    const [expanded, setExpanded] = React.useState(false);
    const [openRenameDialog, setOpenRenameDialog] = React.useState(false);

    const handleChange = (panel) => (event, isExpanded) => {
        setExpanded(isExpanded ? panel : false);
    };

    const handleRenameClick = () => {
        setOpenRenameDialog(true);
    }
    const handleRenameInfo = (inputValue) => {
        setOpenRenameDialog(false);
        if (inputValue) {
            handleRenameCallback(fileItemInfo.name, inputValue);
        }
    }

    const getIconOfFileType = (fileType) => {
        switch (fileType) {
            case 'directory':
                return <FolderIcon />;
            case 'jpg':
            case 'jpeg':
            case 'png':
            case 'gif':
                return <ImageIcon />;
            case 'mp3':
            case 'wav':
            case 'flac':
                return <MusicNoteIcon />;
            case 'txt':
                return <DescriptionIcon />;
            case 'pdf':
                return <InsertDriveFileIcon />;
            case 'js':
            case 'jsx':
            case 'ts':
            case 'tsx':
                return <CodeIcon />;
            default:
                return <FileCopyIcon />;
        }
    };


    return (
        <>
            <div>
                <Accordion expanded={expanded === 'panel1'} onChange={handleChange('panel1')} className='relative rounded-full px-3 py-1 text-sm leading-6 text-gray-600 ring-1 ring-gray-900/10 hover:ring-gray-900/20'>
                    <AccordionSummary
                        expandIcon={<ExpandMoreIcon />}
                        aria-controls="panel1bh-content"
                        id="panel1bh-header"
                    >
                        {getIconOfFileType(fileItemInfo.type)}
                        <Typography sx={{ paddingLeft: 3, flexShrink: 0 }}>
                            {String(fileItemInfo.name)}
                        </Typography>
                        <Typography sx={{ position: 'absolute', left: '50%', color: 'text.secondary' }}>
                            {String(fileItemInfo.lastModified)}
                        </Typography>
                    </AccordionSummary>
                    <AccordionDetails>
                        <CloudDownloadOutlinedIcon
                            style={{ position: 'absolute', top: '20%', right: '5%', cursor: 'pointer', transition: 'all 0.1s ease-in' }}
                            onClick={() => handleDownloadClick(fileItemInfo)} />
                        <DeleteOutlineRoundedIcon
                            style={{ position: 'absolute', top: '50%', right: '5%', cursor: 'pointer', transition: 'all 0.1s ease-ind' }}
                            onClick={() => handleDeleteClick(fileItemInfo)} />
                        <DriveFileRenameOutlineIcon
                            style={{ position: 'absolute', top: '80%', right: '5%', cursor: 'pointer', transition: 'all 0.1s ease-in' }}
                            onClick={() => handleRenameClick(fileItemInfo)} />
                        <Typography sx={{ paddingBottom: 1 }}>
                            name: {String(fileItemInfo.name)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            access: {String(fileItemInfo.access)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            owner: {String(fileItemInfo.owner)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            group: {String(fileItemInfo.group)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            size: {String(fileItemInfo.size)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            lastModified: {String(fileItemInfo.lastModified)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            isDir: {String(fileItemInfo.isDir)}
                        </Typography>
                        <Typography sx={{ paddingBottom: 1 }}>
                            type: {String(fileItemInfo.type)}
                        </Typography>
                    </AccordionDetails>
                </Accordion>

            </div>
            <InputDialog open={openRenameDialog} text={'Please input the new name of the file:'} callback={handleRenameInfo} />
        </>
    );
}
