import * as React from 'react';
import FileItem from './fileItem';

export default function FileList({ fileItems, handleDownloadClick, handleDeleteClick, handleRenameCallback }) {
    return (
        <div>
            {fileItems.map((fileItem, index) => {
                return (
                    <FileItem key={index} fileItemInfo={fileItem} handleDownloadClick={handleDownloadClick} handleDeleteClick={handleDeleteClick} handleRenameCallback={handleRenameCallback}/>
                );
            })}
        </div>
    );
}