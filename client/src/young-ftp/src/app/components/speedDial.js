import * as React from 'react';
import Box from '@mui/material/Box';
import SpeedDial from '@mui/material/SpeedDial';
import SpeedDialIcon from '@mui/material/SpeedDialIcon';
import SpeedDialAction from '@mui/material/SpeedDialAction';
import CloudUploadIcon from '@mui/icons-material/CloudUpload';
import EditIcon from '@mui/icons-material/Edit';
import CreateNewFolderIcon from '@mui/icons-material/CreateNewFolder';
import InputDialog from './inputDialog';


export default function OpenIconSpeedDial({ createNewFolderCallback, uploadCallback }) {
    const [openInputDialog, setOpenInputDialog] = React.useState(false);
    const [inputDialogText, setInputDialogText] = React.useState('');
    const handleCreateNewFolderCallback = (inputValue) => {
        if (inputValue) {
            console.log(inputValue);

            createNewFolderCallback(inputValue);
        }
        setOpenInputDialog(false);
    }
    const handleCreateNewFolder = () => {
        setOpenInputDialog(true);
        setInputDialogText('Please input the name of the new folder:');
    }
    const actions = [
        { icon: <CreateNewFolderIcon />, name: 'New Folder', action: handleCreateNewFolder },
        { icon: <CloudUploadIcon />, name: 'Upload', action: uploadCallback }
    ];
    return (
        <>
            <SpeedDial
                ariaLabel="SpeedDial openIcon example"
                sx={{ position: 'absolute', right: '1%', top: '2%' }}
                icon={<SpeedDialIcon />}
                openIcon={<EditIcon />}
                direction='down'
            >
                {actions.map((action) => (
                    <SpeedDialAction
                        key={action.name}
                        icon={action.icon}
                        tooltipTitle={action.name}
                        onClick={action.action}
                    />
                ))}
            </SpeedDial>
            <InputDialog open={openInputDialog} text={inputDialogText} callback={handleCreateNewFolderCallback} />
        </>
    );
}
