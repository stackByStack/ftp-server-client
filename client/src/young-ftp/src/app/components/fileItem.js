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



export default function FileItem({ fileItemInfo }) {
    const [expanded, setExpanded] = React.useState(false);

    const handleChange = (panel) => (event, isExpanded) => {
        setExpanded(isExpanded ? panel : false);
    };

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
        <div>
            <Accordion expanded={expanded === 'panel1'} onChange={handleChange('panel1')}>
                <AccordionSummary
                    expandIcon={<ExpandMoreIcon />}
                    aria-controls="panel1bh-content"
                    id="panel1bh-header"
                >
                    {getIconOfFileType(fileItemInfo.type)}
                    <Typography sx={{ width: '33%', flexShrink: 0 }}>
                        {String(fileItemInfo.name)}
                    </Typography>
                    <Typography sx={{ color: 'text.secondary' }}>
                        {String(fileItemInfo.lastModified)}
                    </Typography>
                </AccordionSummary>
                <AccordionDetails>
                    <Typography>
                        name: {String(fileItemInfo.name)}
                    </Typography>
                    <Typography>
                        access: {String(fileItemInfo.access)}
                    </Typography>
                    <Typography>
                        owner: {String(fileItemInfo.owner)}
                    </Typography>
                    <Typography>
                        group: {String(fileItemInfo.group)}
                    </Typography>
                    <Typography>
                        size: {String(fileItemInfo.size)}
                    </Typography>
                    <Typography>
                        lastModified: {String(fileItemInfo.lastModified)}
                    </Typography>
                    <Typography>
                        isDir: {String(fileItemInfo.isDir)}
                    </Typography>
                    <Typography>
                        type: {String(fileItemInfo.type)}
                    </Typography>
                </AccordionDetails>
            </Accordion>

        </div>
    );
}
